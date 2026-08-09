# Timestamp timezone support — design

Date: 2026-08-09
Status: approved (brainstorming), pending implementation plan

## Problem

`POST /readings` only accepts `captured_at` values matching a strict UTC-only
regex (`YYYY-MM-DDTHH:MM:SS.sssZ`). Any other valid ISO-8601 shape — a
non-`Z` offset (e.g. `+02:00`), or a naive timestamp with no timezone at all —
is rejected with 400. Separately, the history chart's x-axis tick labels
(`history-chart.js`) hardcode UTC display (`toISOString().slice(11,16)`),
which is wrong for organizers reading the dashboard in Czechia
(Europe/Prague, CET/CEST).

## Why strict validation exists (must be preserved)

`captured_at` is stored as `TEXT` in SQLite (`backend/src/store/db.js`) and
compared **lexicographically**, not as a parsed timestamp:

- `getHistory` / `getLatestCapturedAt`: SQL `ORDER BY captured_at` / `WHERE
  captured_at >= ?` / `MAX(captured_at)`.
- `registerIngestRoutes` (routes.js): JS `reading.capturedAt <
  latestCapturedAtBeforeBatch` to decide `backfilled`.

String comparison only equals chronological comparison if every stored value
is the same canonical shape (same timezone, fixed-width fields). This is why
the current validator hard-rejects anything that isn't exactly UTC `Z` with
3-digit milliseconds — and why the fix must normalize at the boundary rather
than relax the regex.

The firmware (`firmware/src/transmit/hw/clock.cpp`) always emits canonical
UTC `Z` via SNTP + `strftime(..., "%Y-%m-%dT%H:%M:%S.000Z", ...)`, so this
change is about ingest robustness (other/future clients, manual testing),
not a firmware requirement.

## Decisions

- **Naive timestamps** (no `Z`, no offset) are interpreted as **Europe/Prague
  wall-clock time** (user decision), correctly resolving CET (`+01:00`) vs
  CEST (`+02:00`) per calendar date — not hardcoded to one offset.
- **DST transition edge case** (the ~1 hour/year spring-forward gap and
  fall-back overlap) is a known, accepted limitation for naive input: not
  specially resolved, since it cannot occur near the Aug-2026 target and
  isn't worth the added complexity now.
- **Explicit offsets** (`Z` or `±HH:MM`) are accepted as-is and normalized —
  no restriction on which offset.
- Display timezone defaults to **Europe/Prague**, expressed as a single named
  constant so a later per-user-preference override is a one-line change, not
  a refactor.

## Design

### Backend: normalize at ingest

New file `backend/src/ingest/timestamp.js`:

- `normalizeCapturedAt(input)` — returns a canonical `YYYY-MM-DDTHH:MM:SS.sssZ`
  string, or `null` if `input` doesn't parse.
  - Regex gate: `^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(\.\d{1,3})?(Z|[+-]\d{2}:\d{2})?$`
  - If the match has `Z` or an explicit offset: unambiguous instant, so
    `new Date(input).toISOString()` (native `Date` parses any valid ISO
    offset correctly and pads milliseconds).
  - If naive: interpret the date/time components as Europe/Prague wall-clock
    time. Resolve via an `Intl.DateTimeFormat('en-US', { timeZone:
    'Europe/Prague', ... })`-based offset lookup against a UTC guess (format
    the guessed instant back into Prague time, read off the actual offset,
    apply it), then `.toISOString()` the result.
  - Reject (`null`) anything that doesn't match the regex or produces an
    invalid `Date`.

`backend/src/ingest/routes.js` changes:

- Replace `isValidCapturedAt` / `ISO_8601_UTC` with a single pass that maps
  the incoming batch through `normalizeCapturedAt`, 400s if any entry is
  `null`, and uses the **normalized** array for everything downstream: the
  backfill `<` comparison, `insertReading`, and the `wireShape` live-broadcast
  payload. This keeps the stored/compared shape canonical without touching
  `store/readings.js` or the SQL at all.

### Frontend: display in Europe/Prague

New file `backend/src/static/timezone.js`:

- `DISPLAY_TIMEZONE = 'Europe/Prague'`
- `formatLocalTime(ms)` — `Intl.DateTimeFormat` with `timeZone:
  DISPLAY_TIMEZONE, hour: '2-digit', minute: '2-digit', hourCycle: 'h23'`,
  returns `HH:MM`.

`backend/src/static/history-chart.js`: `formatTimeTick` calls
`formatLocalTime(timestampMs)` instead of `new
Date(timestampMs).toISOString().slice(11, 16)`.

This is the only spot in the current UI that renders an absolute clock time
— `dashboard.js`'s "před Xs/min" age display is relative and
timezone-independent, so it's untouched.

Isolating the timezone behind this one named constant/function is what makes
a later per-user override (e.g. read from a settings/localStorage value
instead of the hardcoded constant) a localized change.

## Testing

- New `backend/test/timestamp.test.js` (unit-level, mirrors existing
  `node:test` style, no new deps):
  - Naive Prague timestamp in August → normalizes with `+02:00` (CEST)
    applied.
  - Naive Prague timestamp in January → normalizes with `+01:00` (CET)
    applied — proves the offset lookup isn't hardcoded to one season.
  - Explicit non-UTC offset (e.g. `+05:30`) normalizes correctly.
  - `Z` input passes through canonically (existing behavior preserved).
  - Malformed strings → `null` (covers the existing "space instead of T"
    case and other shapes).
- Extend `backend/test/ingest.test.js`: a batch mixing `Z` and offset
  representations of the same instant is still deduped/ordered correctly
  through the real `/readings` endpoint (the actual regression this whole
  change is about — the original strict-UTC validator existed to protect
  the lexicographic sort from exactly this).
- No test harness exists for static frontend JS (no build step) — verify
  `history-chart.js`'s display change manually in-browser (`npm start`,
  load the dashboard, confirm chart tick labels match Prague local time)
  rather than claiming it works from code inspection alone.

## Out of scope

- Per-user timezone preference override (explicitly deferred by the user;
  this design only creates the seam for it).
- Any firmware change (firmware already emits canonical UTC `Z`).
- Migrating `captured_at` storage away from lexicographically-sortable TEXT
  (out of scope; normalizing at the boundary preserves the existing
  invariant without a schema/query change).
