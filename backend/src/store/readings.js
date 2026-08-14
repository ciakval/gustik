import { startOfLocalDayIso } from './localday.js';

const INSERT_SQL = `
INSERT INTO readings
  (client_id, captured_at, received_at, clock_synced, wind_speed_ms, wind_dir_octant, rssi_dbm, backfilled)
VALUES
  (@clientId, @capturedAt, @receivedAt, @clockSynced, @windSpeedMs, @windDirOctant, @rssiDbm, @backfilled)
ON CONFLICT(client_id) DO NOTHING
`;

function toRow(reading, { receivedAt, backfilled }) {
  return {
    clientId: reading.clientId,
    capturedAt: reading.capturedAt,
    receivedAt,
    clockSynced: reading.clockSynced ? 1 : 0,
    windSpeedMs: reading.windSpeedMs,
    windDirOctant: reading.windDirOctant,
    rssiDbm: reading.rssiDbm ?? null,
    backfilled: backfilled ? 1 : 0,
  };
}

function fromRow(row) {
  if (!row) return null;
  return {
    capturedAt: row.captured_at,
    windSpeedMs: row.wind_speed_ms,
    windDirOctant: row.wind_dir_octant,
    rssiDbm: row.rssi_dbm,
  };
}

// The public wire shape above is deliberately trimmed to what the dashboard
// renders. The diagnostics page (/readings/status) needs the whole row -
// receivedAt/clockSynced/backfilled/clientId are exactly the fields every
// bug so far (bug-028..031) was diagnosed with by hand.
function fullFromRow(row) {
  if (!row) return null;
  return {
    clientId: row.client_id,
    capturedAt: row.captured_at,
    receivedAt: row.received_at,
    clockSynced: row.clock_synced === 1,
    backfilled: row.backfilled === 1,
    windSpeedMs: row.wind_speed_ms,
    windDirOctant: row.wind_dir_octant,
    rssiDbm: row.rssi_dbm,
  };
}

/**
 * Insert a single reading. No-op (idempotent) if clientId already exists.
 * receivedAt/backfilled are the ingest layer's decision, not the caller's data.
 */
export function insertReading(db, reading, { receivedAt = new Date().toISOString(), backfilled = false } = {}) {
  const stmt = db.prepare(INSERT_SQL);
  const info = stmt.run(toRow(reading, { receivedAt, backfilled }));
  return { inserted: info.changes > 0 };
}

export function getLatest(db) {
  const row = db.prepare('SELECT * FROM readings ORDER BY captured_at DESC LIMIT 1').get();
  return fromRow(row);
}

// Snapshot used by the ingest layer to decide backfilled vs live (AD-9) -
// null when the table is empty (first-ever write is never "older than"
// anything, so it's live).
export function getLatestCapturedAt(db) {
  const row = db.prepare('SELECT MAX(captured_at) AS capturedAt FROM readings').get();
  return row.capturedAt ?? null;
}

// Time series covering at minimum the current race day (FR-8), ascending by
// captured_at. `now` is injectable for tests; production callers use the
// default (real current time).
//
// `from`/`to` are canonical ISO-8601 UTC strings and are compared as TEXT -
// see cerebrum.md: every stored captured_at is guaranteed to be exactly
// 'YYYY-MM-DDTHH:MM:SS.sssZ' by the ingest route, which is what makes
// lexicographic comparison equal chronological comparison here. Do not pass
// any other string shape in.
//
// The default window is the current **Europe/Prague** day, not the UTC day -
// the race day starts at local midnight (see store/localday.js).
export function getHistory(db, { now = new Date(), from, to } = {}) {
  const since = from ?? startOfLocalDayIso(now);
  const until = to ?? now.toISOString();
  const rows = db
    .prepare('SELECT * FROM readings WHERE captured_at >= ? AND captured_at <= ? ORDER BY captured_at ASC')
    .all(since, until);
  return rows.map(fromRow);
}

// Same window semantics as getHistory, full rows - diagnostics only.
export function getHistoryFull(db, { now = new Date(), from, to } = {}) {
  const since = from ?? startOfLocalDayIso(now);
  const until = to ?? now.toISOString();
  const rows = db
    .prepare('SELECT * FROM readings WHERE captured_at >= ? AND captured_at <= ? ORDER BY captured_at ASC')
    .all(since, until);
  return rows.map(fullFromRow);
}

export function getLatestFull(db) {
  return fullFromRow(db.prepare('SELECT * FROM readings ORDER BY captured_at DESC LIMIT 1').get());
}

// Newest first - the diagnostics table reads top-down as "what just happened".
export function getRecentFull(db, limit = 30) {
  const rows = db.prepare('SELECT * FROM readings ORDER BY captured_at DESC LIMIT ?').all(limit);
  return rows.map(fullFromRow);
}

export function getTotals(db) {
  const row = db
    .prepare(
      `SELECT
         COUNT(*) AS readings,
         COALESCE(SUM(backfilled), 0) AS backfilled,
         COALESCE(SUM(1 - clock_synced), 0) AS clockUnsynced
       FROM readings`,
    )
    .get();
  return { readings: row.readings, backfilled: row.backfilled, clockUnsynced: row.clockUnsynced };
}
