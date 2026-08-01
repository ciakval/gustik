import { test } from 'node:test';
import assert from 'node:assert/strict';
import { openDb } from '../src/store/db.js';
import { insertReading, getLatest } from '../src/store/readings.js';

function freshDb() {
  return openDb(':memory:');
}

test('getLatest returns null when no readings exist', () => {
  const db = freshDb();
  assert.equal(getLatest(db), null);
});

test('insertReading writes a row retrievable via getLatest', () => {
  const db = freshDb();
  insertReading(db, {
    clientId: 'abc-1',
    capturedAt: '2026-08-01T09:00:00.000Z',
    clockSynced: true,
    windSpeedMs: 4.2,
    windDirOctant: 3,
    rssiDbm: -55,
  });

  const latest = getLatest(db);
  assert.equal(latest.capturedAt, '2026-08-01T09:00:00.000Z');
  assert.equal(latest.windSpeedMs, 4.2);
  assert.equal(latest.windDirOctant, 3);
  assert.equal(latest.rssiDbm, -55);
});

test('insertReading is a no-op on duplicate clientId (idempotent)', () => {
  const db = freshDb();
  insertReading(db, {
    clientId: 'dup-1',
    capturedAt: '2026-08-01T09:00:00.000Z',
    clockSynced: true,
    windSpeedMs: 1,
    windDirOctant: 0,
    rssiDbm: null,
  });
  insertReading(db, {
    clientId: 'dup-1',
    capturedAt: '2026-08-01T09:05:00.000Z',
    clockSynced: true,
    windSpeedMs: 9,
    windDirOctant: 5,
    rssiDbm: -40,
  });

  const count = db.prepare('SELECT COUNT(*) AS n FROM readings').get().n;
  assert.equal(count, 1);
  // first write wins; duplicate must not overwrite
  assert.equal(getLatest(db).windSpeedMs, 1);
});

test('insertReading accepts null rssiDbm (no WiFi scan yet)', () => {
  const db = freshDb();
  insertReading(db, {
    clientId: 'no-rssi',
    capturedAt: '2026-08-01T09:00:00.000Z',
    clockSynced: false,
    windSpeedMs: 0,
    windDirOctant: 0,
    rssiDbm: null,
  });
  assert.equal(getLatest(db).rssiDbm, null);
});
