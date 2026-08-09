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
