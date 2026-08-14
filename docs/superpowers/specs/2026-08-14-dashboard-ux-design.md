# Dashboard UX rework — wind direction, graph resolution, status page

Date: 2026-08-14
Workflow: Superpowers-style scoped change (post-v1, see CLAUDE.md "Choosing a workflow for new work").
Prompted by: Mlok, after running real firmware against the live backend — "the display doesn't look so good".

## Problem

The dashboard shipped in Epic 1/3 is functionally complete but reads badly against real data:

1. **Wind direction is a bare text label.** `octantToCompassLabel()` renders `N/NE/E/SE/S/SW/W/NW` as
   plain text inside an otherwise Czech UI. On the history chart, direction is a scatter series on a
   *linear* right-hand axis 0..7. Wind oscillating around north jumps between the top and the bottom
   of the axis on consecutive samples — the single worst way to plot a circular quantity.
2. **The history graph has no time control at all.** `/readings/history` hardcodes "since start of the
   current **UTC** day" — which in Europe/Prague means the graph resets at 02:00 local, not midnight.
   With a 3 s sample interval that is up to ~28 800 points per day rendered raw into one canvas;
   a live check on 2026-08-14 already returned 2 439 points for a partial day.
3. **No diagnostics view.** Every real bug so far (bug-028 → bug-031) was diagnosed by hand-curling
   the backend and cross-checking a serial capture. Nothing in the UI shows `receivedAt`,
   `clockSynced`, `backfilled`, `clientId`, RSSI over time, or gaps in the data.

## Constraints that shape the design

- **NFR-6 / AD-5: direction resolution is 8 octants, period.** The vane hardware has 8 discrete
  resistance steps. The UI must never render an invented precise degree value. Everything below
  snaps to multiples of 45°.
- Direction is meteorological — *where the wind blows **from*** (the WH1080 vane points into the
  wind). The UI states this explicitly rather than leaving it ambiguous.
- **Consistency convention: SI on the wire.** `/readings/history` never takes a unit parameter;
  m/s ↔ knots stays client-only (FR-10).
- No frontend build step, no CDN. Vanilla ES modules + the single vendored Chart.js UMD bundle.
- Backend is public and unauthenticated (FR-9). The status page adds no new exposure — it renders
  data that `/readings/latest` and `/readings/history` already serve publicly.

## Decisions (confirmed with Mlok 2026-08-14)

| Decision | Choice | Why |
|---|---|---|
| Compass labels | **Czech** `S/SV/V/JV/J/JZ/Z/SZ` | Matches the rest of the Czech UI and the scout-organizer audience. The `S` = *sever* vs English `S` = *south* trap is mitigated by always printing the full word (`severozápad`) next to the abbreviation. |
| Aggregated speed | **Average line + min–max gust band** | Gusts are the actual safety signal (P550 capsize risk). A plain average at 1-minute buckets hides exactly the peak that matters. |
| Status page discoverability | **Small footer link** next to "Návod k obsluze" | Backend is public anyway; hiding it buys nothing and costs reachability from a phone on the boat. |

## Design

### 1. Wind direction

**Pure module `static/compass.js`** (unit-tested, no DOM):

- `OCTANT_LABELS_CS = ['S','SV','V','JV','J','JZ','Z','SZ']` indexed by octant 0..7.
- `OCTANT_NAMES_CS` — full words (`sever`, `severovýchod`, …).
- `octantToDegrees(octant) = octant * 45` — exact multiples of 45 only, never interpolated.
- `octantArrowRotation(octant)` — rotation for an arrow glyph that points *along the wind's travel*
  (i.e. `from + 180°`), which is what reads correctly on a map-like rose.
- `circularMeanOctant(octants)` — vector mean over unit vectors, snapped back to the nearest octant.
  Used for bucket aggregation on the backend too (same algorithm, duplicated deliberately: backend
  has no import path into `static/`). Returns `null` for an empty input.

**Live card** — an inline SVG compass rose:

- Fixed 8 tick marks with Czech labels around the ring (S at top).
- A single arrow snapped to the current octant, drawn *from* the reported direction pointing to the
  rose centre — visually "the wind comes from there".
- Underneath: `SZ` (large) + `severozápad` (small) + the explicit qualifier `vítr fouká od`.
- The rose is static markup; only the arrow's `transform: rotate()` changes per update. No degrees
  are ever printed.

**History chart** — direction becomes a strip of arrow glyphs pinned to the top of the plot area:

- A second dataset on the same chart, on a hidden `dir` y-axis (`min 0, max 1`), all points at
  `y = 0.92`, `showLine: false`, `pointStyle: 'triangle'`, per-point `rotation` from
  `octantArrowRotation()`.
- Chart.js rotates point styles natively, so wraparound is structurally impossible — an arrow at
  N and an arrow at NNW-ish differ by 45° of rotation, not by a jump across an axis.
- Arrows are decimated to at most ~24 across the visible range so they never overlap.

### 2. Time range and resolution (Grafana-style)

**Backend — `GET /readings/history` gains three optional query parameters:**

| Param | Meaning | Default |
|---|---|---|
| `from` | ISO-8601 UTC instant, inclusive | start of the current **Europe/Prague** day (was: UTC day) |
| `to` | ISO-8601 UTC instant, inclusive | now |
| `bucket` | bucket width in **seconds**; `0`/absent = raw rows | absent |

- With no parameters at all the endpoint is **byte-for-byte backwards compatible** with the shipped
  contract except for the day boundary fix (raw rows, `{capturedAt, windSpeedMs, windDirOctant, rssiDbm}`).
- With `bucket`, each element is
  `{capturedAt, windSpeedMs, windSpeedMinMs, windSpeedMaxMs, windDirOctant, rssiDbm, sampleCount}`
  where `capturedAt` is the bucket's **start** instant, `windSpeedMs` is the mean, and
  `windDirOctant` is the circular mean snapped to an octant. `rssiDbm` is the mean, or `null` if
  every sample in the bucket had `null`.
- Empty buckets are **omitted**, not zero-filled — a gap in the data must look like a gap, not like
  a calm. `sampleCount` lets the client tell a thin bucket from a full one.
- The server clamps the bucket count to **2 000** per response (raises `bucket` if needed) and
  rejects a `from`/`to` it cannot parse with 400, same discipline as the ingest route.
- Aggregation lives in a **pure module** `src/serve/aggregate.js` (`bucketReadings`), tested
  independently of the DB and HTTP layers.

**Frontend — `static/timerange.js`** (pure, unit-tested):

- `RANGES` — `15 min`, `1 h`, `3 h`, `6 h`, `12 h`, `dnes` (Prague day). **Default `1 h`.**
- `autoBucketSeconds(rangeSeconds, targetPoints = 180)` — snaps to a "nice" ladder
  (3, 5, 10, 15, 30, 60, 120, 300, 600, 900, 1800, 3600 s) so tick labels stay round. 3 s is the
  floor (the firmware's own sample interval — finer buckets cannot add information).
- Resolution is **auto by default and overridable**, exactly like Grafana's "interval" control:
  a `<select>` offering `auto` plus the ladder entries that make sense for the chosen range.
  The effective resolution is always shown as text (`rozlišení 30 s`) so an overridden value is
  never silently in effect.
- Selection persists in `localStorage` (`gustik.range`, `gustik.bucket`) so a phone reopening the
  page keeps its view.

The chip row + resolution select are rendered by a shared `static/timerange-ui.js` and mounted on
**both** the dashboard and the status page, per Mlok's "duplicate it here".

### 3. Status page

**Backend — new `GET /readings/status`:**

```
{
  serverTimeIso,
  latest: {                     // full row, not the trimmed wire shape
    clientId, capturedAt, receivedAt, clockSynced, backfilled,
    windSpeedMs, windDirOctant, rssiDbm
  } | null,
  totals: { readings, backfilled, clockUnsynced },
  recent: [ ...full rows, newest first, `?limit=` (default 30, max 200) ],
  ingestEvents: [ ...in-memory POST log, newest first ],
  gaps: [ { fromIso, toIso, seconds } ]   // over ?from/?to, threshold 3× median interval
}
```

- **`ingestEvents` is an in-memory ring buffer** (last 100) populated by the ingest route:
  `{atIso, count, inserted, duplicates, backfilled, remoteAddress}`. This is the one thing the
  status page genuinely cannot derive from the DB, and it is exactly what bug-031 needed —
  a POST whose rows all collide on `client_id` shows up as `inserted: 0, duplicates: N` instead of
  vanishing silently. It resets on restart, by design (no schema change, no new table).
- **`gaps`** are derived from `captured_at` deltas in the requested window: any delta larger than
  3× the median delta (floor 15 s) is reported. This is what "the device was offline" looks like
  from the backend's side.

**Frontend — `static/status.html` + `status.js`:**

1. **Latest reading** — every field, with age, `clockSynced` / `backfilled` / staleness rendered as
   coloured badges rather than raw booleans.
2. **RSSI chart** — `rssiDbm` over the selected range, same aggregation pipeline as the speed chart.
   Reference bands at −67 dBm (good) / −80 dBm (marginal) so a number that means nothing to a
   non-engineer still reads as "fine / not fine".
3. **Event log** — `ingestEvents` and `gaps` merged into one reverse-chronological table.
4. **Recent readings** — raw rows, newest first, full detail.
5. Same time-range chips as the dashboard.

### 4. `POST /readings` response contract (added on Mlok's follow-up ask)

The endpoint used to answer `201 {written: <number received>}` unconditionally. `written` counted
what *arrived*, not what was *stored* — and since `client_id` is UNIQUE with
`ON CONFLICT DO NOTHING` (the Story 2.2 backfill-retry safety net), a batch can be discarded in full
with no trace. That conflation is exactly why bug-031 stayed hidden: the device reported `sent=yes`
every cycle for hours while nothing landed, and the HTTP response could not have told it otherwise.

New contract:

| Status | Meaning |
|---|---|
| `401` | bad or missing token |
| `400` | nothing stored **and the batch is at fault** — empty batch, or an unparseable `capturedAt`. Rejected whole, never partly. |
| `201` | at least one row stored |
| `200` | batch understood and fully accounted for, but **nothing new stored** — every `clientId` was already known |

Body on every 2xx: `{received, inserted, duplicates, backfilled}`, always satisfying
`received == inserted + duplicates`. The `200` case adds an explicit `warning` string. `written` is
gone rather than kept as an alias — a misleadingly-named field next to accurate ones is worse than
no field, and its only producer was this line (the firmware ignores the body entirely).

Duplicates deliberately stay **2xx**: they are not a transport failure. The data is on the server,
retrying changes nothing, and a non-2xx would make the firmware re-buffer rows that are already
safely stored (`main.cpp` only clears its flash buffer on a 2xx). What changed is that it is no
longer a *201 Created* when nothing was created.

Empty batches now `400`: both firmware call sites are guarded (`main.cpp` sends either exactly one
reading or a buffer it has just checked is non-empty), so an empty POST means a caller bug and a
2xx would hide it.

## Out of scope

- **The firmware does not yet read any of this.** `WifiTransmitClient::send()` checks only
  `statusCode >= 200 && < 300`, so the new counts are visible via `/status.html` and `curl` but not
  in the device's own Serial diagnostics. Making the firmware parse `inserted` and report a
  "sent but stored nothing" state on Serial is the natural next step — it needs a rebuild and
  reflash to have any effect, so it is a separate change.
- No persistence for `ingestEvents` — deliberately in-memory only.
- No auth on the status page — consistent with FR-9's public, no-login dashboard.

## Verification

- `npm test` (backend, `node:test`) — aggregation, range params, status endpoint, compass and
  timerange pure modules.
- Real headless-Chromium render of `/` and `/status.html` against a live server seeded with
  realistic data (3 s samples, a deliberate gap, a backfilled batch), asserting non-blank canvases
  and zero console errors — the check pattern established in Story 3.2.
