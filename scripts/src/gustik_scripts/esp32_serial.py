"""
esp32_serial.py

Reads the raw magnetometer stream produced by the ESP32 bring-up sketch
`firmware/src/diag/mag_diag.cpp`, which prints one line per sample:

    MAG <x> <y> <z>

Stdlib only, on purpose. pyserial is not installed for this machine's system
python, and opening a port through it toggles DTR/RTS, which resets the ESP32
mid-capture. Plain open() + termios is what `stty ... raw` + `cat` does: it
attaches to an already-running board without resetting it.

Anything that is not a sample line - boot-loader noise, the '#' banner, a
half-line from attaching mid-stream, an I2C error report - is skipped rather
than raising. A capture has to survive garbage without losing the samples
around it.

Every live capture is also written verbatim to a raw log. That is not
belt-and-braces: the same wind-vane capture that confirmed one calibration
later answered a question nobody had asked when it was taken. Raw data can be
re-questioned; a derived number cannot.
"""

import os
import select
import termios
import time

__all__ = [
    "SAMPLE_PREFIX",
    "SerialLineReader",
    "capture_samples",
    "iter_samples",
    "parse_sample",
    "read_capture_file",
]

SAMPLE_PREFIX = "MAG"
DEFAULT_PORT = "/dev/ttyUSB0"
DEFAULT_BAUD = 115200

# termios has no portable int -> Bnnn mapping, and only the rates this
# project's firmware actually uses are worth carrying.
_BAUD_CONSTANTS = {
    9600: termios.B9600,
    38400: termios.B38400,
    57600: termios.B57600,
    115200: termios.B115200,
    230400: termios.B230400,
}


def parse_sample(line):
    """
    Parse one "MAG <x> <y> <z>" line into an (x, y, z) tuple of ints.

    Returns None for any line that is not a well-formed sample, so callers
    can filter a noisy stream with a single `is not None` check.
    """
    parts = line.split()
    if len(parts) != 4 or parts[0] != SAMPLE_PREFIX:
        return None
    try:
        return tuple(int(part) for part in parts[1:])
    except ValueError:
        return None


def iter_samples(lines):
    """Yield (x, y, z) for every parsable sample line in `lines`."""
    for line in lines:
        sample = parse_sample(line)
        if sample is not None:
            yield sample


def read_capture_file(path):
    """
    Re-read a previously saved raw capture. Lets a capture be re-analysed
    with a hypothesis you did not have when you took it, without touching
    the hardware again.
    """
    with open(path, errors="replace") as handle:
        return list(iter_samples(handle))


class SerialLineReader:
    """
    Line-buffered reader for a serial port already streaming text.

    Use it as a context manager. `read_lines()` returns whatever complete
    lines have arrived; `latest_sample()` drains the backlog and returns only
    the newest reading, which is what you want when the number has to reflect
    where the board is pointing *now* rather than a second ago.
    """

    def __init__(self, port=DEFAULT_PORT, baud=DEFAULT_BAUD):
        if baud not in _BAUD_CONSTANTS:
            raise ValueError(f"baud must be one of {sorted(_BAUD_CONSTANTS)}")
        self.port = port
        self.baud = baud
        self._buffer = ""
        # O_NONBLOCK matters at open() time too: without it, opening a tty
        # blocks until carrier is detected, which for a USB-serial bridge can
        # hang indefinitely.
        self._fd = os.open(port, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
        try:
            self._configure()
        except Exception:
            os.close(self._fd)
            raise

    def _configure(self):
        iflag, oflag, cflag, lflag, _ispeed, _ospeed, cc = termios.tcgetattr(self._fd)
        speed = _BAUD_CONSTANTS[self.baud]

        # Raw mode, spelled out rather than via cfmakeraw(): that helper lives
        # in `tty`, not `termios`, and only from Python 3.12. This tool is meant
        # to run under whatever python3 is on the machine, so it does not get to
        # depend on which.
        iflag &= ~(
            termios.IGNBRK | termios.BRKINT | termios.PARMRK | termios.ISTRIP
            | termios.INLCR | termios.IGNCR | termios.ICRNL | termios.IXON
        )
        oflag &= ~termios.OPOST
        lflag &= ~(
            termios.ECHO | termios.ECHONL | termios.ICANON | termios.ISIG
            | termios.IEXTEN
        )
        cflag &= ~(termios.CSIZE | termios.PARENB)
        cflag |= termios.CS8
        # CLOCAL: ignore modem control lines, so a bridge that never asserts
        # DCD still reads. CREAD: actually enable the receiver. Clearing
        # HUPCL keeps DTR asserted on close, so exiting does not reset the
        # ESP32 and end the stream for whatever reads it next.
        cflag |= termios.CLOCAL | termios.CREAD
        cflag &= ~termios.HUPCL

        cc = list(cc)
        cc[termios.VMIN] = 0
        cc[termios.VTIME] = 0

        termios.tcsetattr(
            self._fd,
            termios.TCSANOW,
            [iflag, oflag, cflag, lflag, speed, speed, cc],
        )
        termios.tcflush(self._fd, termios.TCIFLUSH)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def close(self):
        if self._fd is not None:
            os.close(self._fd)
            self._fd = None

    def read_lines(self, timeout=1.0):
        """
        Return the complete lines available within `timeout` seconds.

        A partial trailing line is kept buffered for the next call, so a read
        that lands mid-line does not corrupt the sample it split.
        """
        ready, _, _ = select.select([self._fd], [], [], timeout)
        if not ready:
            return []
        try:
            chunk = os.read(self._fd, 65536)
        except BlockingIOError:
            return []
        if not chunk:
            return []
        self._buffer += chunk.decode("ascii", errors="replace")
        *lines, self._buffer = self._buffer.split("\n")
        return [line.rstrip("\r") for line in lines]

    def latest_sample(self, timeout=2.0):
        """
        Drain everything buffered and return the newest sample, or None if
        none arrived within `timeout`.

        Reading one line at a time would hand back the oldest queued sample
        instead, so a slow caller would lag further behind reality with every
        call - fatal for --check-rotation, where the reading has to track the
        board as it is turned.
        """
        deadline = time.monotonic() + timeout
        newest = None
        while True:
            for line in self.read_lines(timeout=0.05):
                sample = parse_sample(line)
                if sample is not None:
                    newest = sample
            if newest is not None or time.monotonic() >= deadline:
                return newest


def capture_samples(reader, seconds, raw_log=None, progress=None):
    """
    Read for `seconds`, returning every sample seen.

    Reads continuously rather than sampling on a timer: the ESP32 sets the
    pace, so draining as fast as lines arrive both keeps the OS buffer empty
    and collects the most data. `raw_log` is an open text file every line is
    mirrored to verbatim, garbage included.
    """
    samples = []
    start = time.monotonic()
    last_report = -1.0
    while True:
        elapsed = time.monotonic() - start
        if elapsed >= seconds:
            break
        for line in reader.read_lines(timeout=0.2):
            if raw_log is not None:
                raw_log.write(line + "\n")
            sample = parse_sample(line)
            if sample is not None:
                samples.append(sample)
        if progress is not None and elapsed - last_report >= 1.0:
            progress(elapsed, seconds, len(samples))
            last_report = elapsed
    if raw_log is not None:
        raw_log.flush()
    return samples
