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
