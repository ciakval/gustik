import { test } from 'node:test';
import assert from 'node:assert/strict';
import { buildApp } from '../src/app.js';

function testApp() {
  return buildApp({ dbPath: ':memory:', ingestToken: 'secret-token' });
}

async function postReadings(app, readings) {
  return app.inject({
    method: 'POST',
    url: '/readings',
    headers: { authorization: 'Bearer secret-token' },
    payload: readings,
  });
}

test('GET /readings/history returns today\'s records ascending by capturedAt, in wire shape', async () => {
  const app = testApp();
  const today = new Date().toISOString().slice(0, 10);
  await postReadings(app, [
    { clientId: 'h-2', capturedAt: `${today}T10:00:00.000Z`, clockSynced: true, windSpeedMs: 2, windDirOctant: 1, rssiDbm: -55 },
    { clientId: 'h-1', capturedAt: `${today}T09:00:00.000Z`, clockSynced: true, windSpeedMs: 1, windDirOctant: 0, rssiDbm: -50 },
  ]);

  const res = await app.inject({ method: 'GET', url: '/readings/history' });
  assert.equal(res.statusCode, 200);
  const { readings } = JSON.parse(res.body);
  assert.equal(readings.length, 2);
  assert.deepEqual(
    readings.map((r) => r.capturedAt),
    [`${today}T09:00:00.000Z`, `${today}T10:00:00.000Z`],
  );
  assert.deepEqual(Object.keys(readings[0]).sort(), ['capturedAt', 'rssiDbm', 'windDirOctant', 'windSpeedMs']);
});

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

test('GET /readings/history excludes records from a previous day', async () => {
  const app = testApp();
  const today = new Date().toISOString().slice(0, 10);
  const yesterday = new Date(Date.now() - 24 * 60 * 60 * 1000).toISOString().slice(0, 10);
  await postReadings(app, [
    { clientId: 'old-1', capturedAt: `${yesterday}T09:00:00.000Z`, clockSynced: true, windSpeedMs: 9, windDirOctant: 0, rssiDbm: -50 },
    { clientId: 'today-1', capturedAt: `${today}T09:00:00.000Z`, clockSynced: true, windSpeedMs: 1, windDirOctant: 0, rssiDbm: -50 },
  ]);

  const res = await app.inject({ method: 'GET', url: '/readings/history' });
  const { readings } = JSON.parse(res.body);
  assert.equal(readings.length, 1);
  assert.equal(readings[0].windSpeedMs, 1);
});

test('GET /readings/history ignores a unit query parameter - always SI (Consistency Conventions)', async () => {
  const app = testApp();
  const today = new Date().toISOString().slice(0, 10);
  await postReadings(app, [
    { clientId: 'h-1', capturedAt: `${today}T09:00:00.000Z`, clockSynced: true, windSpeedMs: 5, windDirOctant: 0, rssiDbm: -50 },
  ]);

  const res = await app.inject({ method: 'GET', url: '/readings/history?unit=knots' });
  const { readings } = JSON.parse(res.body);
  assert.equal(readings[0].windSpeedMs, 5);
});

test('GET /readings/history returns null rssiDbm without crashing (AC3)', async () => {
  const app = testApp();
  const today = new Date().toISOString().slice(0, 10);
  await postReadings(app, [
    { clientId: 'h-1', capturedAt: `${today}T09:00:00.000Z`, clockSynced: false, windSpeedMs: 5, windDirOctant: 0, rssiDbm: null },
  ]);

  const res = await app.inject({ method: 'GET', url: '/readings/history' });
  assert.equal(res.statusCode, 200);
  const { readings } = JSON.parse(res.body);
  assert.equal(readings[0].rssiDbm, null);
});

test('GET /readings/history returns an empty array when there is no data', async () => {
  const app = testApp();
  const res = await app.inject({ method: 'GET', url: '/readings/history' });
  assert.equal(res.statusCode, 200);
  // The response also echoes the window/bucket it used (see history-range.test.js);
  // `readings` is the part this assertion is about.
  assert.deepEqual(JSON.parse(res.body).readings, []);
});
