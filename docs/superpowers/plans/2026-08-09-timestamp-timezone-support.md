# Timestamp Timezone Support Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Accept timezone-aware (`Z` or `±HH:MM` offset) and naive (assumed Europe/Prague) ISO-8601 timestamps at the `/readings` ingest endpoint, normalizing everything to canonical UTC before storage, and display the history chart's time axis in Europe/Prague instead of hardcoded UTC.

**Architecture:** A new `normalizeCapturedAt()` function in `backend/src/ingest/timestamp.js` replaces the current strict-UTC-only regex validator in `routes.js`; it accepts a wider set of valid ISO-8601 shapes and converts every one of them to the same canonical `YYYY-MM-DDTHH:MM:SS.sssZ` shape the rest of the system (SQLite's lexicographic `ORDER BY`/`WHERE`, the JS backfill `<` comparison) already depends on — so no other file in the chain changes behavior. On the frontend, a new `backend/src/static/timezone.js` module centralizes the Europe/Prague display timezone behind one named constant/function, and `history-chart.js`'s x-axis tick formatter is pointed at it instead of `toISOString()`.

**Tech Stack:** Node.js 24, `node:test` (built-in, no new deps), native `Intl.DateTimeFormat` for timezone conversion (no date library needed — Node ships full ICU).

**Spec:** `docs/superpowers/specs/2026-08-09-timestamp-timezone-support-design.md`

---

## Task 1: `normalizeCapturedAt` — backend timestamp normalization module

**Files:**
- Create: `backend/src/ingest/timestamp.js`
- Test: `backend/test/timestamp.test.js`

- [ ] **Step 1: Write the failing tests**

Create `backend/test/timestamp.test.js`:

```js
import { test } from 'node:test';
import assert from 'node:assert/strict';
import { normalizeCapturedAt } from '../src/ingest/timestamp.js';

test('normalizeCapturedAt passes a canonical UTC "Z" timestamp through unchanged', () => {
  assert.equal(normalizeCapturedAt('2026-08-01T09:00:00.000Z'), '2026-08-01T09:00:00.000Z');
});

test('normalizeCapturedAt accepts a non-UTC explicit offset and converts to UTC', () => {
  // +05:30 (IST) - 5.5 hours ahead of UTC
  assert.equal(normalizeCapturedAt('2026-08-01T14:30:00.000+05:30'), '2026-08-01T09:00:00.000Z');
});

test('normalizeCapturedAt accepts an offset with no milliseconds and pads to .000', () => {
  assert.equal(normalizeCapturedAt('2026-08-01T09:00:00Z'), '2026-08-01T09:00:00.000Z');
});

test('normalizeCapturedAt treats a naive timestamp in August as Europe/Prague summer time (CEST, +02:00)', () => {
  assert.equal(normalizeCapturedAt('2026-08-01T09:00:00'), '2026-08-01T07:00:00.000Z');
});

test('normalizeCapturedAt treats a naive timestamp in January as Europe/Prague winter time (CET, +01:00), not hardcoded CEST', () => {
  assert.equal(normalizeCapturedAt('2026-01-01T09:00:00'), '2026-01-01T08:00:00.000Z');
});

test('normalizeCapturedAt returns null for a non-ISO shape (space instead of "T", no zone)', () => {
  assert.equal(normalizeCapturedAt('2026-08-01 09:00:00'), null);
});

test('normalizeCapturedAt returns null for a non-string input', () => {
  assert.equal(normalizeCapturedAt(undefined), null);
  assert.equal(normalizeCapturedAt(12345), null);
});

test('normalizeCapturedAt returns null for garbage that merely starts with digits', () => {
  assert.equal(normalizeCapturedAt('2026-08-01T09:00:00.000Xyz'), null);
});
```

- [ ] **Step 2: Run tests to verify they fail**

Run: `cd backend && node --test test/timestamp.test.js`
Expected: FAIL — `Cannot find module '../src/ingest/timestamp.js'`

- [ ] **Step 3: Write the implementation**

Create `backend/src/ingest/timestamp.js`:

```js
// Naive timestamps (no Z, no offset) are interpreted as this IANA zone.
// Centralized here so backend interpretation and frontend display
// (backend/src/static/timezone.js) can be kept in sync by eye.
const NAIVE_INPUT_TIMEZONE = 'Europe/Prague';

const CAPTURED_AT_PATTERN =
  /^(\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2})(?:\.(\d{1,3}))?(Z|[+-]\d{2}:\d{2})?$/;

const pragueFormatter = new Intl.DateTimeFormat('en-US', {
  timeZone: NAIVE_INPUT_TIMEZONE,
  year: 'numeric',
  month: '2-digit',
  day: '2-digit',
  hour: '2-digit',
  minute: '2-digit',
  second: '2-digit',
  hourCycle: 'h23',
});

function partsToUtcMs(parts) {
  return Date.UTC(
    Number(parts.year),
    Number(parts.month) - 1,
    Number(parts.day),
    Number(parts.hour),
    Number(parts.minute),
    Number(parts.second),
  );
}

// How would `instantMs` be rendered as a Prague wall clock, reinterpreted
// as if that rendering were itself UTC? Used below to measure Prague's
// offset from UTC at a given instant.
function renderAsIfUtcMs(instantMs) {
  const parts = Object.fromEntries(
    pragueFormatter.formatToParts(new Date(instantMs)).map((part) => [part.type, part.value]),
  );
  return partsToUtcMs(parts);
}

// Given wall-clock components meant as Europe/Prague local time, find the
// UTC instant they refer to. First guess treats the components as if they
// were already UTC, then corrects by how far that guess's Prague rendering
// drifted from the target - converges in one correction outside the
// ~1hr/year DST transition window (spring-forward gap / fall-back overlap
// is a known, accepted ambiguity - see design doc, not specially handled).
function pragueWallClockToUtcMs(year, month, day, hour, minute, second, ms) {
  const guessMs = Date.UTC(year, month - 1, day, hour, minute, second, ms);
  const offsetMs = renderAsIfUtcMs(guessMs) - guessMs;
  return guessMs - offsetMs;
}

/**
 * Normalizes a captured-at timestamp to canonical UTC
 * (YYYY-MM-DDTHH:MM:SS.sssZ), or returns null if it doesn't parse.
 *
 * Accepts: explicit "Z", any explicit +/-HH:MM offset, or naive (no zone -
 * interpreted as Europe/Prague wall-clock time). Storage and every query
 * that reads captured_at back (SQLite ORDER BY/WHERE, the ingest layer's
 * backfill `<` comparison) rely on every stored value sharing this exact
 * canonical shape for their lexicographic comparisons to equal chronological
 * ones - see the design doc for why.
 */
export function normalizeCapturedAt(input) {
  if (typeof input !== 'string') return null;

  const match = CAPTURED_AT_PATTERN.exec(input);
  if (!match) return null;

  const [, year, month, day, hour, minute, second, fraction, zone] = match;
  const ms = fraction ? Number(fraction.padEnd(3, '0')) : 0;

  if (zone) {
    const date = new Date(input);
    return Number.isNaN(date.getTime()) ? null : date.toISOString();
  }

  const utcMs = pragueWallClockToUtcMs(
    Number(year),
    Number(month),
    Number(day),
    Number(hour),
    Number(minute),
    Number(second),
    ms,
  );
  const date = new Date(utcMs);
  return Number.isNaN(date.getTime()) ? null : date.toISOString();
}
```

- [ ] **Step 4: Run tests to verify they pass**

Run: `cd backend && node --test test/timestamp.test.js`
Expected: PASS (8 tests)

- [ ] **Step 5: Commit**

```bash
git add backend/src/ingest/timestamp.js backend/test/timestamp.test.js
git commit -m "feat: add normalizeCapturedAt for timezone-aware ingest timestamps"
```

---

## Task 2: Wire normalization into the ingest route

**Files:**
- Modify: `backend/src/ingest/routes.js`
- Modify: `backend/test/ingest.test.js`
- Modify: `backend/test/history.test.js`

- [ ] **Step 1: Write the failing tests**

In `backend/test/ingest.test.js`, replace the existing "non-ISO-8601" 400 test and add new acceptance tests. Find this block:

```js
test('POST /readings with a non-ISO-8601 capturedAt (e.g. space instead of "T", no "Z") returns 400 and writes nothing', async () => {
  const app = testApp();
  const res = await app.inject({
    method: 'POST',
    url: '/readings',
    headers: { authorization: 'Bearer secret-token' },
    payload: [{ ...VALID_READING, capturedAt: '2026-08-01 09:00:00' }],
  });
  assert.equal(res.statusCode, 400);
  assert.equal(getLatest(app.db), null);
});
```

Keep it as-is (it still exercises real rejection behavior), and add immediately after it:

```js
test('POST /readings with an explicit non-UTC offset capturedAt is accepted and normalized to UTC', async () => {
  const app = testApp();
  const res = await app.inject({
    method: 'POST',
    url: '/readings',
    headers: { authorization: 'Bearer secret-token' },
    payload: [{ ...VALID_READING, clientId: 'r-offset', capturedAt: '2026-08-01T11:00:00.000+02:00' }],
  });
  assert.ok(res.statusCode >= 200 && res.statusCode < 300, `expected 2xx, got ${res.statusCode}`);
  assert.equal(getLatest(app.db).capturedAt, '2026-08-01T09:00:00.000Z');
});

test('POST /readings with a naive capturedAt is accepted and normalized as Europe/Prague local time', async () => {
  const app = testApp();
  const res = await app.inject({
    method: 'POST',
    url: '/readings',
    headers: { authorization: 'Bearer secret-token' },
    payload: [{ ...VALID_READING, clientId: 'r-naive', capturedAt: '2026-08-01T11:00:00' }],
  });
  assert.ok(res.statusCode >= 200 && res.statusCode < 300, `expected 2xx, got ${res.statusCode}`);
  // August in Prague is CEST (+02:00): 11:00 local -> 09:00 UTC
  assert.equal(getLatest(app.db).capturedAt, '2026-08-01T09:00:00.000Z');
});
```

In `backend/test/history.test.js`, add a new test after the first one (`'GET /readings/history returns today\'s records ascending by capturedAt, in wire shape'`):

```js
test('GET /readings/history sorts correctly when records mix "Z" and explicit-offset capturedAt for the same day', async () => {
  const app = testApp();
  const today = new Date().toISOString().slice(0, 10);
  await postReadings(app, [
    // 11:00 in UTC+02:00 == 09:00 UTC - earlier than the next record despite
    // sorting later as a raw string if offsets weren't normalized away.
    { clientId: 'h-offset', capturedAt: `${today}T11:00:00.000+02:00`, clockSynced: true, windSpeedMs: 1, windDirOctant: 0, rssiDbm: -50 },
    { clientId: 'h-utc', capturedAt: `${today}T10:00:00.000Z`, clockSynced: true, windSpeedMs: 2, windDirOctant: 1, rssiDbm: -55 },
  ]);

  const res = await app.inject({ method: 'GET', url: '/readings/history' });
  const { readings } = JSON.parse(res.body);
  assert.deepEqual(
    readings.map((r) => r.windSpeedMs),
    [1, 2],
  );
});
```

- [ ] **Step 2: Run tests to verify the new ones fail**

Run: `cd backend && node --test test/ingest.test.js test/history.test.js`
Expected: the two new `ingest.test.js` tests FAIL with 400 (current strict-UTC-only regex rejects both); the new `history.test.js` test FAILS (offset timestamp currently rejected with 400 by the POST inside `postReadings`, so the GET assertion never gets real data — confirm by checking the POST's status in the test run if needed)

- [ ] **Step 3: Update the implementation**

In `backend/src/ingest/routes.js`, replace the top of the file (the `ISO_8601_UTC` regex, `isValidCapturedAt`, and the validation/usage inside the handler):

```js
import { insertReading, getLatestCapturedAt } from '../store/readings.js';
import { broadcastReading, broadcastHistoryChanged } from '../serve/routes.js';
import { normalizeCapturedAt } from './timestamp.js';

function isAuthorized(request, token) {
  const header = request.headers.authorization ?? '';
  return header === `Bearer ${token}`;
}

function wireShape(reading) {
  return {
    capturedAt: reading.capturedAt,
    windSpeedMs: reading.windSpeedMs,
    windDirOctant: reading.windDirOctant,
    rssiDbm: reading.rssiDbm ?? null,
  };
}

export function registerIngestRoutes(fastify, { ingestToken }) {
  fastify.post('/readings', async (request, reply) => {
    if (!isAuthorized(request, ingestToken)) {
      return reply.code(401).send({ error: 'unauthorized' });
    }

    const rawReadings = Array.isArray(request.body) ? request.body : [request.body];

    // getHistory (readings.js) filters by lexicographically comparing
    // captured_at against a generated 'YYYY-MM-DDTHH:MM:SS.sssZ' cutoff -
    // any other shape silently sorts wrong and drops the row from history
    // without erroring anywhere. normalizeCapturedAt() converts every
    // accepted shape (explicit "Z", explicit offset, or naive-as-Prague-
    // local) to that one canonical shape; readings whose capturedAt can't
    // be parsed at all become null here and 400 the whole batch below.
    const readings = rawReadings.map((reading) => ({
      ...reading,
      capturedAt: normalizeCapturedAt(reading.capturedAt),
    }));

    const invalid = readings.find((reading) => reading.capturedAt === null);
    if (invalid) {
      return reply.code(400).send({ error: 'invalid capturedAt: must be ISO-8601, e.g. 2026-08-01T09:00:00.000Z or 2026-08-01T11:00:00+02:00' });
    }

    // Snapshot once per request (AD-9: "older than the last SO FAR
    // received" at the moment the batch arrives) - every record in this
    // batch is compared against the same baseline, not against each other.
    const latestCapturedAtBeforeBatch = getLatestCapturedAt(fastify.db);

    let anyBackfilled = false;
    for (const reading of readings) {
      const backfilled = latestCapturedAtBeforeBatch !== null && reading.capturedAt < latestCapturedAtBeforeBatch;
      const { inserted } = insertReading(fastify.db, reading, { backfilled });
      if (inserted) {
        if (backfilled) {
          anyBackfilled = true;
        } else {
          broadcastReading(fastify, wireShape(reading));
        }
      }
    }
    if (anyBackfilled) {
      broadcastHistoryChanged(fastify);
    }

    return reply.code(201).send({ written: readings.length });
  });
}
```

- [ ] **Step 4: Run tests to verify they pass**

Run: `cd backend && node --test`
Expected: PASS, all tests (existing 43 + new ones from Task 1 and this task)

- [ ] **Step 5: Commit**

```bash
git add backend/src/ingest/routes.js backend/test/ingest.test.js backend/test/history.test.js
git commit -m "feat: accept timezone-aware and naive capturedAt timestamps at ingest"
```

---

## Task 3: Display history chart times in Europe/Prague

**Files:**
- Create: `backend/src/static/timezone.js`
- Modify: `backend/src/static/history-chart.js:13-15`

- [ ] **Step 1: Write the implementation**

There's no browser test harness for static JS (see spec's Testing section — verified manually in Task 4 instead). Create `backend/src/static/timezone.js`:

```js
// Single named seam for the dashboard's display timezone. A future
// per-user-preference override (e.g. read from localStorage) only needs to
// change what this resolves to - see CLAUDE.md "Choosing a workflow" /
// the timestamp-timezone-support design doc.
export const DISPLAY_TIMEZONE = 'Europe/Prague';

const timeFormatter = new Intl.DateTimeFormat('en-GB', {
  timeZone: DISPLAY_TIMEZONE,
  hour: '2-digit',
  minute: '2-digit',
  hourCycle: 'h23',
});

export function formatLocalTime(timestampMs) {
  return timeFormatter.format(new Date(timestampMs));
}
```

- [ ] **Step 2: Wire it into the chart**

In `backend/src/static/history-chart.js`, add the import at the top:

```js
import { buildSpeedPoints, buildDirectionPoints } from './history-chart-data.js';
import { formatLocalTime } from './timezone.js';
```

Replace the `formatTimeTick` function:

```js
function formatTimeTick(timestampMs) {
  return formatLocalTime(timestampMs);
}
```

- [ ] **Step 3: Run the existing test suite to confirm nothing broke**

Run: `cd backend && node --test`
Expected: PASS, same test count as end of Task 2 (this task adds no automated tests — static frontend JS has no test harness in this project)

- [ ] **Step 4: Commit**

```bash
git add backend/src/static/timezone.js backend/src/static/history-chart.js
git commit -m "feat: display history chart times in Europe/Prague instead of UTC"
```

---

## Task 4: Manual browser verification

**Files:** none (verification only)

- [ ] **Step 1: Start the backend**

Run: `cd backend && INGEST_TOKEN=test-token npm start`
Expected: server logs it's listening (default port per `src/index.js`)

- [ ] **Step 2: Post a reading with a naive timestamp near "now" in Prague local time**

In a separate terminal, compute the current Prague wall-clock time and post it:

```bash
curl -s -X POST http://localhost:3000/readings \
  -H "Authorization: Bearer test-token" \
  -H "Content-Type: application/json" \
  -d "[{\"clientId\":\"manual-1\",\"capturedAt\":\"$(TZ=Europe/Prague date +%Y-%m-%dT%H:%M:%S)\",\"clockSynced\":true,\"windSpeedMs\":3.5,\"windDirOctant\":2,\"rssiDbm\":-60}]"
```

Expected: `{"written":1}` with a 2xx status (use `-i` to see the status line if it's not obvious from the body).

- [ ] **Step 3: Load the dashboard in a browser and confirm the chart tick reads Prague local time**

Open `http://localhost:3000/` (or wherever `index.html` is served from — check `src/index.js`/`app.js` for the actual static-serving path if unsure). Confirm:
- The history chart renders a point for the reading just posted.
- The x-axis tick near that point matches the current Prague wall-clock time (`date +%H:%M` with `TZ=Europe/Prague`), not UTC.

- [ ] **Step 4: Stop the server**

Stop the `npm start` process (Ctrl-C or kill the background job).

No commit for this task — verification only.

---

## Task 5: OpenWolf bookkeeping

**Files:**
- Modify: `.wolf/anatomy.md` (regenerated, not hand-edited)
- Modify: `.wolf/STATUS.md`
- Modify: `.wolf/memory.md`

- [ ] **Step 1: Regenerate anatomy.md**

Run: `openwolf scan`
Expected: `.wolf/anatomy.md` picks up the three new files (`backend/src/ingest/timestamp.js`, `backend/test/timestamp.test.js`, `backend/src/static/timezone.js`) with descriptions/token counts.

- [ ] **Step 2: Update STATUS.md**

Add a line under `✅ Concluído`/done section (or wherever the most recent completed work is tracked) noting: timestamp/timezone ingest support shipped (naive timestamps interpreted as Europe/Prague, explicit offsets accepted, history chart now displays Europe/Prague instead of UTC) — this was Superpowers-flow work (see spec/plan under `docs/superpowers/`), not a new BMAD story, per the "Choosing a workflow" section in `CLAUDE.md`.

- [ ] **Step 3: Append to memory.md**

Add a one-line entry per the OPENWOLF.md format:
`| HH:MM | timezone-aware capturedAt ingest + Prague display | backend/src/ingest/timestamp.js, routes.js, static/timezone.js, history-chart.js | done, tests passing | ~<estimate> tokens |`

- [ ] **Step 4: Commit**

```bash
git add .wolf/anatomy.md .wolf/STATUS.md .wolf/memory.md
git commit -m "chore: OpenWolf bookkeeping for timestamp/timezone ingest work"
```

---

## Self-Review Notes

- **Spec coverage:** normalization module (Task 1), ingest wiring + mixed-offset ordering regression test (Task 2), display seam + chart wiring (Task 3), manual browser verification per the spec's testing section (Task 4) — all covered. OpenWolf bookkeeping (Task 5) isn't in the spec but is a standing project-level requirement (`.claude/rules/openwolf.md`), included here since this plan is the natural place to not forget it.
- **Placeholder scan:** no TBD/TODO; every step has literal code or an exact command with expected output.
- **Type/name consistency:** `normalizeCapturedAt` (Task 1) is the exact name imported and called in Task 2; `formatLocalTime`/`DISPLAY_TIMEZONE` (Task 3) match between the module and its one call site. `NAIVE_INPUT_TIMEZONE` (backend, Task 1) and `DISPLAY_TIMEZONE` (frontend, Task 3) are intentionally two separate constants in two separate module graphs (server vs static browser JS, no shared bundle) — both hardcode `'Europe/Prague'` today; a comment in each points at the other so a future edit doesn't update one and forget the other.
