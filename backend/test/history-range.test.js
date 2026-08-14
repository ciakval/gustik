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

// A fixed window in the past keeps these tests independent of "today",
// which the default (no-params) behaviour is already covered for elsewhere.
const DAY = '2026-08-14';
function at(hhmmss, extra = {}) {
  return {
    clientId: `r-${hhmmss}`,
    capturedAt: `${DAY}T${hhmmss}.000Z`,
    clockSynced: true,
    windSpeedMs: 1,
    windDirOctant: 0,
    rssiDbm: -60,
    ...extra,
  };
}

test('GET /readings/history honours explicit from/to bounds, inclusive', async () => {
  const app = testApp();
  await postReadings(app, [at('09:00:00'), at('10:00:00'), at('11:00:00')]);

  const res = await app.inject({
    method: 'GET',
    url: `/readings/history?from=${DAY}T10:00:00.000Z&to=${DAY}T11:00:00.000Z`,
  });
  assert.equal(res.statusCode, 200);
  const { readings } = JSON.parse(res.body);
  assert.deepEqual(
    readings.map((r) => r.capturedAt),
    [`${DAY}T10:00:00.000Z`, `${DAY}T11:00:00.000Z`],
  );
});

test('GET /readings/history normalizes a non-canonical from bound instead of comparing it as-is', async () => {
  // 'YYYY-MM-DDTHH:MMZ' parses fine but sorts wrong against the stored
  // 'YYYY-MM-DDTHH:MM:SS.sssZ' text - see cerebrum.md / bug-019.
  const app = testApp();
  await postReadings(app, [at('09:00:00'), at('11:00:00')]);

  const res = await app.inject({
    method: 'GET',
    url: `/readings/history?from=${DAY}T10:00Z&to=${DAY}T23:00:00.000Z`,
  });
  const { readings, from } = JSON.parse(res.body);
  assert.equal(from, `${DAY}T10:00:00.000Z`);
  assert.equal(readings.length, 1);
  assert.equal(readings[0].capturedAt, `${DAY}T11:00:00.000Z`);
});

test('GET /readings/history rejects an unparseable from bound with 400', async () => {
  const app = testApp();
  const res = await app.inject({ method: 'GET', url: '/readings/history?from=yesterday' });
  assert.equal(res.statusCode, 400);
  assert.match(JSON.parse(res.body).error, /from\/to/);
});

test('GET /readings/history with bucket returns aggregated points with a gust band', async () => {
  const app = testApp();
  await postReadings(app, [
    at('10:00:00', { windSpeedMs: 2 }),
    at('10:00:30', { windSpeedMs: 8 }),
    at('10:01:00', { windSpeedMs: 4 }),
  ]);

  const res = await app.inject({
    method: 'GET',
    url: `/readings/history?from=${DAY}T10:00:00.000Z&to=${DAY}T10:02:00.000Z&bucket=60`,
  });
  const { readings, bucketSeconds } = JSON.parse(res.body);
  assert.equal(bucketSeconds, 60);
  assert.equal(readings.length, 2);
  assert.equal(readings[0].windSpeedMs, 5);
  assert.equal(readings[0].windSpeedMinMs, 2);
  assert.equal(readings[0].windSpeedMaxMs, 8);
  assert.equal(readings[0].sampleCount, 2);
});

test('GET /readings/history clamps a too-fine bucket and reports the effective width', async () => {
  const app = testApp();
  await postReadings(app, [at('00:00:00')]);

  // 3s buckets over a full day would be 28800 points, past the 2000 cap.
  const res = await app.inject({
    method: 'GET',
    url: `/readings/history?from=${DAY}T00:00:00.000Z&to=${DAY}T23:59:59.000Z&bucket=3`,
  });
  const { bucketSeconds } = JSON.parse(res.body);
  assert.ok(bucketSeconds > 3, `expected a coarser bucket, got ${bucketSeconds}`);
  assert.ok(86400 / bucketSeconds <= 2000);
});

test('GET /readings/history treats bucket=auto and bucket=0 as raw rows', async () => {
  const app = testApp();
  await postReadings(app, [at('10:00:00'), at('10:00:30')]);

  for (const bucket of ['auto', '0']) {
    const res = await app.inject({
      method: 'GET',
      url: `/readings/history?from=${DAY}T10:00:00.000Z&to=${DAY}T11:00:00.000Z&bucket=${bucket}`,
    });
    const { readings, bucketSeconds } = JSON.parse(res.body);
    assert.equal(bucketSeconds, 0);
    assert.equal(readings.length, 2);
    assert.equal(readings[0].sampleCount, undefined, 'raw rows carry no aggregation fields');
  }
});

test('GET /readings/history rejects a negative bucket with 400', async () => {
  const app = testApp();
  const res = await app.inject({ method: 'GET', url: '/readings/history?bucket=-5' });
  assert.equal(res.statusCode, 400);
});

test('GET /readings/history echoes the window it actually used when none was given', async () => {
  const app = testApp();
  const res = await app.inject({ method: 'GET', url: '/readings/history' });
  const { from, to } = JSON.parse(res.body);
  assert.match(from, /^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{3}Z$/);
  assert.match(to, /^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{3}Z$/);
  assert.ok(Date.parse(from) <= Date.parse(to));
});
