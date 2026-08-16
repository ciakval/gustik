#pragma once

#include <cstdint>

// Default GPIO assignments for the status LED panel, each overridable from
// platformio.ini without editing source:
//
//     build_flags = -DGUSTIK_PANEL_PIN_STATUS_RED=32
//
// The board is a 38-pin "ESP32 DevKit v1 / NodeMCU-32S" style module
// (ESP32-D0WD-V3, 4 MB flash, CP2102) - probed over serial, not assumed.
// The whole status group plus both sensors plus a GND plus the button is
// one contiguous run down the LEFT header column; the detail group is a run
// down the RIGHT column.
//
//   left column, top -> bottom:  ... 34 35 32 33 25 26 27 14 12 GND 13 ...
//   right column, top -> bottom: ... 19 18  5 17 16  4  0  2 15 ...
//
// The status group runs DOWN the left column and the detail group runs UP
// the right one - see the as-built note below.
//
// GPIO25/26 are TAKEN OVER from the 2026-08-11 bring-up diagnostics
// (`config loaded` / `WiFi connected`). No LED was ever physically fitted to
// GPIO2, 25 or 26 and nobody ever read one, so this is a reassignment rather
// than a migration. GPIO2 (the onboard LED, Story 2.4 / FR-5) is unchanged,
// which keeps the disconnect requirement satisfied with the panel absent,
// disabled or asleep.
//
// Deliberately avoided: GPIO5/0/15/2 (strapping pins - GPIO5 sits physically
// between 18 and 17 on the header and is SKIPPED, which is the one thing
// about this pin map that looks like a wiring mistake and is not), GPIO12
// (must read low at boot), GPIO34-39 (input only), and GPIO6-11.
//
//     WARNING: GPIO6-11 are broken out on this board as CLK/SD0/SD1/SD2/
//     SD3/CMD at the bottom of both columns. They are the SPI flash bus:
//     anything connected there crashes the chip or corrupts flash. `V5` -
//     where the battery pack's + wire lands - sits immediately next to
//     `CMD` (GPIO11). One stray strand there puts 6.4 V on a flash pin.

#ifndef GUSTIK_PANEL_PIN_STATUS_RED
#define GUSTIK_PANEL_PIN_STATUS_RED 32
#endif
#ifndef GUSTIK_PANEL_PIN_STATUS_YELLOW
#define GUSTIK_PANEL_PIN_STATUS_YELLOW 33
#endif
#ifndef GUSTIK_PANEL_PIN_STATUS_GREEN
#define GUSTIK_PANEL_PIN_STATUS_GREEN 25
#endif
#ifndef GUSTIK_PANEL_PIN_STATUS_BLUE
#define GUSTIK_PANEL_PIN_STATUS_BLUE 26
#endif

// AS BUILT 2026-08-16: the detail group runs UP the right column, not down
// it - GPIO4 is the leftmost (green) position and GPIO19 the rightmost
// (red). The two groups therefore traverse the header in opposite
// directions, which looks like a mistake in this list and is not: what has
// to be in order is the PHYSICAL ROW, and it is.
//
// If the panel is ever rebuilt, either order is fine - reverse these five
// lines to match whichever way the jumpers actually run, and confirm with
// `pio run -e panel_diag -t upload`, which walks the row and makes a
// transposition obvious by eye.
#ifndef GUSTIK_PANEL_PIN_DETAIL_1
#define GUSTIK_PANEL_PIN_DETAIL_1 4
#endif
#ifndef GUSTIK_PANEL_PIN_DETAIL_2
#define GUSTIK_PANEL_PIN_DETAIL_2 16
#endif
#ifndef GUSTIK_PANEL_PIN_DETAIL_3
#define GUSTIK_PANEL_PIN_DETAIL_3 17
#endif
#ifndef GUSTIK_PANEL_PIN_DETAIL_4
#define GUSTIK_PANEL_PIN_DETAIL_4 18
#endif
#ifndef GUSTIK_PANEL_PIN_DETAIL_5
#define GUSTIK_PANEL_PIN_DETAIL_5 19
#endif

#ifndef GUSTIK_PANEL_PIN_BUTTON
#define GUSTIK_PANEL_PIN_BUTTON 13
#endif
