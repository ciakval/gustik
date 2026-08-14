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

function todayAt(hhmmss, extra = {}) {
  const day = new Date().toISOString().slice(0, 10);
  return {
    clientId: `s-${hhmmss}`,
    capturedAt: `${day}T${hhmmss}.000Z`,
    clockSynced: true,
    windSpeedMs: 3,
    windDirOctant: 2,
    rssiDbm: -70,
    ...extra,
  };
}

test('GET /readings/status exposes the full latest row, not the trimmed wire shape', async () => {
  const app = testApp();
  await postReadings(app, [todayAt('09:00:00', { clientId: 'AA:BB-123', clockSynced: false })]);

  const res = await app.inject({ method: 'GET', url: '/readings/status' });
  assert.equal(res.statusCode, 200);
  const { latest } = JSON.parse(res.body);
  assert.equal(latest.clientId, 'AA:BB-123');
  assert.equal(latest.clockSynced, false);
  assert.equal(latest.backfilled, false);
  assert.match(latest.receivedAt, /^\d{4}-\d{2}-\d{2}T/);
  assert.equal(latest.windSpeedMs, 3);
});

test('GET /readings/status returns null latest and zero totals on an empty database', async () => {
  const app = testApp();
  const res = await app.inject({ method: 'GET', url: '/readings/status' });
  const body = JSON.parse(res.body);
  assert.equal(body.latest, null);
  assert.deepEqual(body.totals, { readings: 0, backfilled: 0, clockUnsynced: 0 });
  assert.deepEqual(body.recent, []);
  assert.deepEqual(body.gaps, []);
});

test('GET /readings/status counts backfilled and clock-unsynced readings', async () => {
  const app = testApp();
  await postReadings(app, [todayAt('10:00:00')]);
  // Older than what's already stored => backfilled (AD-9).
  await postReadings(app, [todayAt('09:00:00', { clockSynced: false })]);

  const { totals } = JSON.parse((await app.inject({ method: 'GET', url: '/readings/status' })).body);
  assert.deepEqual(totals, { readings: 2, backfilled: 1, clockUnsynced: 1 });
});

test('GET /readings/status lists recent readings newest first, honouring limit', async () => {
  const app = testApp();
  await postReadings(app, [todayAt('09:00:00'), todayAt('09:00:03'), todayAt('09:00:06')]);

  const res = await app.inject({ method: 'GET', url: '/readings/status?limit=2' });
  const { recent } = JSON.parse(res.body);
  assert.equal(recent.length, 2);
  assert.ok(recent[0].capturedAt > recent[1].capturedAt);
});

test('GET /readings/status logs ingest batches newest first, including silently-dropped duplicates', async () => {
  const app = testApp();
  await postReadings(app, [todayAt('09:00:00'), todayAt('09:00:03')]);
  // Same clientIds again - ON CONFLICT DO NOTHING drops both, but the POST
  // still answers 201. This is the bug-031 signature and the whole reason
  // this log exists.
  await postReadings(app, [todayAt('09:00:00'), todayAt('09:00:03')]);

  const { ingestEvents } = JSON.parse((await app.inject({ method: 'GET', url: '/readings/status' })).body);
  assert.equal(ingestEvents.length, 2);
  assert.deepEqual(
    { count: ingestEvents[0].count, inserted: ingestEvents[0].inserted, duplicates: ingestEvents[0].duplicates },
    { count: 2, inserted: 0, duplicates: 2 },
  );
  assert.equal(ingestEvents[1].inserted, 2);
  assert.match(ingestEvents[0].atIso, /^\d{4}-\d{2}-\d{2}T/);
});

test('GET /readings/status reports a data gap once the cadence is established', async () => {
  const app = testApp();
  await postReadings(app, [
    todayAt('09:00:00'),
    todayAt('09:00:03'),
    todayAt('09:00:06'),
    todayAt('09:00:09'),
    // ~6 minute outage
    todayAt('09:06:00'),
    todayAt('09:06:03'),
  ]);

  const { gaps } = JSON.parse((await app.inject({ method: 'GET', url: '/readings/status' })).body);
  assert.equal(gaps.length, 1);
  assert.equal(gaps[0].seconds, 351);
  assert.match(gaps[0].fromIso, /T09:00:09/);
  assert.match(gaps[0].toIso, /T09:06:00/);
});

test('GET /readings/status reports no gaps for a steady cadence', async () => {
  const app = testApp();
  await postReadings(app, [todayAt('09:00:00'), todayAt('09:00:03'), todayAt('09:00:06'), todayAt('09:00:09')]);
  const { gaps } = JSON.parse((await app.inject({ method: 'GET', url: '/readings/status' })).body);
  assert.deepEqual(gaps, []);
});

test('GET /readings/status rejects an unparseable window with 400', async () => {
  const app = testApp();
  const res = await app.inject({ method: 'GET', url: '/readings/status?from=never' });
  assert.equal(res.statusCode, 400);
});
