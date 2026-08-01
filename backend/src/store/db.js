import Database from 'better-sqlite3';

const SCHEMA = `
CREATE TABLE IF NOT EXISTS readings (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  client_id TEXT NOT NULL UNIQUE,
  captured_at TEXT NOT NULL,
  received_at TEXT NOT NULL,
  clock_synced INTEGER NOT NULL,
  wind_speed_ms REAL NOT NULL,
  wind_dir_octant INTEGER NOT NULL,
  rssi_dbm INTEGER,
  backfilled INTEGER NOT NULL DEFAULT 0
);
CREATE INDEX IF NOT EXISTS idx_readings_captured_at ON readings (captured_at);
`;

export function openDb(path) {
  const db = new Database(path);
  if (path !== ':memory:') {
    db.pragma('journal_mode = WAL');
  }
  db.exec(SCHEMA);
  return db;
}
