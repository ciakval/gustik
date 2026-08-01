import { test } from 'node:test';
import assert from 'node:assert/strict';
import { buildApp } from '../src/app.js';
import { getLatest } from '../src/store/readings.js';

function testApp() {
  return buildApp({ dbPath: ':memory:', ingestToken: 'secret-token' });
}

const VALID_READING = {
  clientId: 'r-1',
  capturedAt: '2026-08-01T09:00:00.000Z',
  clockSynced: true,
  windSpeedMs: 3.5,
  windDirOctant: 2,
  rssiDbm: -60,
};

test('POST /readings without a token returns 401 and writes nothing', async () => {
  const app = testApp();
  const res = await app.inject({
    method: 'POST',
    url: '/readings',
    payload: [VALID_READING],
  });
  assert.equal(res.statusCode, 401);
  assert.equal(getLatest(app.db), null);
});

test('POST /readings with an invalid token returns 401 and writes nothing', async () => {
  const app = testApp();
  const res = await app.inject({
    method: 'POST',
    url: '/readings',
    headers: { authorization: 'Bearer wrong-token' },
    payload: [VALID_READING],
  });
  assert.equal(res.statusCode, 401);
  assert.equal(getLatest(app.db), null);
});

test('POST /readings with a valid token and one record writes it and returns 2xx', async () => {
  const app = testApp();
  const res = await app.inject({
    method: 'POST',
    url: '/readings',
    headers: { authorization: 'Bearer secret-token' },
    payload: [VALID_READING],
  });
  assert.ok(res.statusCode >= 200 && res.statusCode < 300, `expected 2xx, got ${res.statusCode}`);
  assert.equal(getLatest(app.db).windSpeedMs, 3.5);
});

test('POST /readings with a repeated clientId is a no-op and still returns success', async () => {
  const app = testApp();
  await app.inject({
    method: 'POST',
    url: '/readings',
    headers: { authorization: 'Bearer secret-token' },
    payload: [VALID_READING],
  });
  const res = await app.inject({
    method: 'POST',
    url: '/readings',
    headers: { authorization: 'Bearer secret-token' },
    payload: [{ ...VALID_READING, windSpeedMs: 99 }],
  });
  assert.ok(res.statusCode >= 200 && res.statusCode < 300);
  // first write wins - duplicate must not overwrite
  assert.equal(getLatest(app.db).windSpeedMs, 3.5);
});

test('POST /readings accepts an array of N records (backfill shape)', async () => {
  const app = testApp();
  const res = await app.inject({
    method: 'POST',
    url: '/readings',
    headers: { authorization: 'Bearer secret-token' },
    payload: [
      VALID_READING,
      { ...VALID_READING, clientId: 'r-2', capturedAt: '2026-08-01T09:00:05.000Z', windSpeedMs: 4.0 },
    ],
  });
  assert.ok(res.statusCode >= 200 && res.statusCode < 300);
  assert.equal(getLatest(app.db).windSpeedMs, 4.0);
});
