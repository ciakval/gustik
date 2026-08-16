# Flash memory on the Station's ESP32 — what exists and what uses it

**Written 2026-08-16**, when the partition table was changed from the Arduino
default to `firmware/partitions_gustik.csv`. All figures below were read off
the actual attached board and the actual build, not from datasheets.

---

## 1. There is one flash chip, not two

This is the first thing to get straight, because it is easy to assume
otherwise: **the module has a single 4 MB SPI NOR flash chip, and everything
persistent lives on it** — the bootloader, the firmware, the WiFi calibration
store and the filesystem holding `config.txt`. There is no separate "program
flash" and "data flash". The division between code and data is made by a
*partition table* written to that one chip, not by hardware.

What the device actually has, probed from the attached board with
`esptool.py flash_id`:

| Memory | Size | Volatile? | What it holds |
|---|---:|---|---|
| **External SPI flash** (in the WROOM module can) | **4 MB** | no | bootloader, partition table, firmware, NVS, LittleFS — everything in §2 |
| Internal SRAM | 520 KB | **yes** | running code's heap/stack. Build reports 15.8 % used |
| Internal ROM | 448 KB | no | first-stage boot ROM, burned at the factory, not writable |
| RTC slow memory | 8 KB | survives deep sleep | unused by this firmware |
| eFuse | — | write-once | MAC `30:76:f5:b9:13:04`, and the **ADC VRef calibration** that makes `analogReadMilliVolts()` accurate — see the battery-sense design |

Chip: **ESP32-D0WD-V3 rev 3.1**, 40 MHz crystal, no embedded PSRAM.
Flash chip manufacturer `0x5e` (Zbit), device `0x4016` = 4 MB.

> If you ever move to a WROVER module, GPIO16/17 stop being free — they are
> wired to the PSRAM chip there. On this WROOM they are available.

---

## 2. How the 4 MB is divided

### Before (Arduino `default.csv`)

```
nvs       data  nvs      0x9000   0x5000   =   20 KB
otadata   data  ota      0xe000   0x2000   =    8 KB
app0      app   ota_0    0x10000  0x140000 = 1.25 MB   <- firmware ran here, 92.4% full
app1      app   ota_1    0x150000 0x140000 = 1.25 MB   <- reserved for OTA. NEVER USED.
spiffs    data  spiffs   0x290000 0x160000 = 1.375 MB  <- LittleFS
coredump  data  coredump 0x3F0000 0x10000  =   64 KB
```

The two application slots exist so a device can download a new firmware image
into the *inactive* slot and switch over — over-the-air updates. **This project
has no OTA mechanism and is not going to get one**: the Station is reflashed
over USB with `pio run -t upload`. So 1.25 MB of a 4 MB chip was permanently
reserved for a feature that does not exist, while the slot actually in use sat
at 92.4 % and every new feature had to be argued against the flash budget.

### After (`firmware/partitions_gustik.csv`)

```
nvs       data  nvs      0x9000   0x5000   =   20 KB
otadata   data  ota      0xe000   0x2000   =    8 KB
app0      app   ota_0    0x10000  0x200000 = 2 MB      <- firmware, now 57.7% full
spiffs    data  spiffs   0x210000 0x1E0000 = 1.875 MB  <- LittleFS, +0.5 MB
coredump  data  coredump 0x3F0000 0x10000  =   64 KB
```

`0x210000 + 0x1E0000 + 0x10000 = 0x400000` — the table fills the chip exactly.

**Measured effect of the change** (`pio run -e esp32dev`, same source):

| | Before | After |
|---|---:|---:|
| Firmware size | 1 210 461 B | 1 210 461 B (unchanged, as it must be) |
| App partition | 1 310 720 B | 2 097 152 B |
| **Flash used** | **92.4 %** | **57.7 %** |
| Headroom | ~98 KB | ~866 KB |
| Filesystem | 1.375 MB | 1.875 MB |

### Two choices in that table that look wrong and are not

- **`app0` keeps subtype `ota_0`, not `factory`.** The Arduino toolchain
  flashes `boot_app0.bin` to `otadata` on every upload, which tells the
  bootloader to boot `ota_0`. Keeping the subtype means the boot path is
  byte-for-byte the one already in use and known to work; a single slot is
  what makes OTA impossible, not the subtype. A `factory` slot would boot only
  via the bootloader's fallback path — a behaviour change for no gain.
- **The data partition is still named `spiffs` although it holds LittleFS.**
  This is not a leftover. `LittleFS.begin()` in
  `framework-arduinoespressif32` defaults to `partitionLabel = "spiffs"`
  (see `libraries/LittleFS/src/LittleFS.h`). Renaming the partition to
  `littlefs` makes `LittleFS.begin()` fail to find it, and `config.txt` plus
  the offline buffer become silently unreadable — the same class of failure as
  bug-028, where the image was built as SPIFFS instead of LittleFS.

---

## 3. What actually uses the filesystem

The `spiffs` partition holds a **LittleFS** image. It is not raw flash access
and it is not SPIFFS — `firmware/platformio.ini` sets
`board_build.filesystem = littlefs` (added by bug-028), and both users below
call `LittleFS.begin()`.

| Path | Written by | Read by | Size |
|---|---|---|---|
| `/config.txt` | `pio run -t uploadfs` from `firmware/data/` | `config/hw/config_loader.cpp` at boot | ~1 KB |
| `/buf/<n>.txt` | `transmit/hw/flash_buffer.cpp`, one file per buffered reading | on reconnect, for backfill | ~110 B each, up to 4800 files |

`NVS` (20 KB, separate partition) is not ours — the ESP-IDF WiFi stack writes
RF calibration data there. Nothing in this firmware opens it.

`coredump` (64 KB) is unused; Arduino builds do not enable flash core dumps by
default. It is kept because it costs nothing and would be genuinely useful the
next time the firmware hangs the way bug-030 did.

---

## 4. Flagged: the 4-hour offline buffer probably does not fit — bug-060

**This is arithmetic, not a measurement.** It is recorded here because §3's
table is where the problem is visible.

`main.cpp` calls `flashBuffer.begin(computeBufferCapacityForHours(4.0, 3.0))`
= **4800 slots**, to satisfy NFR-4's "buffer at least 4 hours locally".
`FlashBuffer::push()` writes **one file per reading** into a single `/buf`
directory.

Each record is ~110 bytes, which is below LittleFS's inline threshold, so the
files do *not* each consume a 4 KB block — that much is fine. But every inline
file's content, name and metadata tags live in the directory's metadata pairs,
each pair costing two 4 KB blocks. 4800 entries at roughly 150 B of metadata
each is ~700 KB of content spread across ~175 metadata pairs, or **~1.4 MB** —
which would not have fitted the old 1.375 MB partition at all, and fits the new
1.875 MB one with little margin.

The capacity is not even the worst part. LittleFS rewrites and compacts
directory metadata as entries are added, so `push()` gets slower as the
directory grows — and `loop()` is required never to block (design constraint
C2). Thousands of files in one directory is the shape of the problem.

**Not fixed here.** The obvious fix is to stop using one file per reading: a
single file of fixed-width records with the existing ring index would be
~614 KB total and O(1) per write. The cheap measurement that would settle it
first: leave the Station running with the backend unreachable and watch how
long it actually buffers, and how long `push()` takes as it fills.

---

## 5. Changing the partition table — what it costs

A partition change moves the filesystem's offset, so **the previously uploaded
`config.txt` is left behind at the old address**. `LittleFS.begin()` is called
with `formatOnFail=true`, which means the device will quietly format the new,
empty partition and boot with *no configuration* — no WiFi, no backend token —
rather than telling you anything is wrong.

So a table change is always these two steps, in this order:

```sh
cd firmware
~/.platformio/penv/bin/pio run -e esp32dev -t upload     # firmware + new table
~/.platformio/penv/bin/pio run -e esp32dev -t uploadfs   # config.txt into the new partition
```

Verify afterwards on the serial monitor: the boot line should report the config
as loaded and the device should associate to WiFi. If it reports no config, the
`uploadfs` step is what is missing.

To read back the table actually on a device or in a build:

```sh
~/.platformio/penv/bin/python \
  ~/.platformio/packages/framework-arduinoespressif32/tools/gen_esp32part.py \
  .pio/build/esp32dev/partitions.bin
```
