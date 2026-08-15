from .calibration import MagCalibration
from .orientation import Orientation

__all__ = ["QMC5883P", "Orientation", "MagCalibration", "main"]


# QMC5883P is imported lazily because qmc5883p.py requires smbus2, which is
# only needed for the USB-I2C bench rig. The ESP32 serial path
# (gustik_scripts.mag_calibrate) is stdlib-only and must stay importable with
# a bare `python3` on a machine that has never installed smbus2 -- which an
# eager import here would prevent.
def __getattr__(name):
    if name in ("QMC5883P", "main"):
        from . import qmc5883p

        return getattr(qmc5883p, name)
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")


def __dir__():
    return sorted(__all__)
