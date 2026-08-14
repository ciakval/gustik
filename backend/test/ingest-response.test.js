import { test } from 'node:test';
import assert from 'node:assert/strict';
import { buildApp } from '../src/app.js';

function testApp() {
  return buildApp({ dbPath: ':memory:', ingestToken: 'secret-token' });
}

async function postReadings(app, payload) {
  return app.inject({
    method: 'POST',
    url: '/readings',
    headers: { authorization: 'Bearer secret-token' },
    payload,
  });
}

function reading(id, hhmmss, extra = {}) {
  const day = new Date().toISOString().slice(0, 10);
  return {
    clientId: id,
    capturedAt: `${day}T${hhmmss}.000Z`,
    clockSynced: true,
    windSpeedMs: 3,
    windDirOctant: 2,
    rssiDbm: -70,
    ...extra,
  };
}

test('a fully accepted batch answers 201 with an accurate per-row breakdown', async () => {
  const app = testApp();
  const res = await postReadings(app, [reading('a', '09:00:00'), reading('b', '09:00:03')]);
  assert.equal(res.statusCode, 201);
  assert.deepEqual(JSON.parse(res.body), { received: 2, inserted: 2, duplicates: 0, backfilled: 0 });
});

test('a single (non-array) reading is reported with the same counted shape', async () => {
  const app = testApp();
  const res = await postReadings(app, reading('solo', '09:00:00'));
  assert.equal(res.statusCode, 201);
  assert.deepEqual(JSON.parse(res.body), { received: 1, inserted: 1, duplicates: 0, backfilled: 0 });
});

test('a batch where nothing was stored answers 200, not 201, and says why', async () => {
  // This is the bug-031 signature: every clientId already exists, so
  // ON CONFLICT(client_id) DO NOTHING discards the whole batch. The old
  // response was an unconditional 201 {written: 2}, which is why the device
  // could report "sent=yes" for hours while the backend stored nothing.
  // Duplicates are NOT a transport error - the data IS on the server and
  // retrying is pointless - so this stays 2xx and the firmware still clears
  // its buffer. It is just not a 201 Created, because nothing was created.
  const app = testApp();
  await postReadings(app, [reading('dup-1', '09:00:00'), reading('dup-2', '09:00:03')]);

  const res = await postReadings(app, [reading('dup-1', '09:00:00'), reading('dup-2', '09:00:03')]);
  assert.equal(res.statusCode, 200);
  const body = JSON.parse(res.body);
  assert.equal(body.received, 2);
  assert.equal(body.inserted, 0);
  assert.equal(body.duplicates, 2);
  assert.match(body.warning, /clientId/i);
});

test('a partially duplicated batch still answers 201 and counts both halves', async () => {
  // The normal backfill-retry case (Story 2.2): re-sending a buffer whose
  // leading rows already landed is expected, not an error.
  const app = testApp();
  await postReadings(app, [reading('p-1', '09:00:00')]);

  const res = await postReadings(app, [reading('p-1', '09:00:00'), reading('p-2', '09:00:03')]);
  assert.equal(res.statusCode, 201);
  const body = JSON.parse(res.body);
  assert.deepEqual(
    { received: body.received, inserted: body.inserted, duplicates: body.duplicates },
    { received: 2, inserted: 1, duplicates: 1 },
  );
  assert.equal(body.warning, undefined, 'a partial insert is not a warning');
});

test('backfilled rows are counted separately from live ones', async () => {
  const app = testApp();
  await postReadings(app, [reading('newest', '10:00:00')]);

  // Older than what is already stored => backfilled (AD-9).
  const res = await postReadings(app, [reading('older-1', '09:00:00'), reading('older-2', '09:00:03')]);
  assert.equal(res.statusCode, 201);
  const body = JSON.parse(res.body);
  assert.equal(body.inserted, 2);
  assert.equal(body.backfilled, 2);
});

test('an empty batch is rejected rather than silently reported as success', async () => {
  // The firmware never sends one (both call sites are guarded), so an empty
  // POST means a caller bug - answering 2xx would hide it.
  const app = testApp();
  const res = await postReadings(app, []);
  assert.equal(res.statusCode, 400);
  assert.match(JSON.parse(res.body).error, /empty/i);
});

test('a malformed capturedAt still 400s the whole batch, with no partial write', async () => {
  const app = testApp();
  const res = await postReadings(app, [reading('ok-1', '09:00:00'), { ...reading('bad', '09:00:03'), capturedAt: 'nonsense' }]);
  assert.equal(res.statusCode, 400);

  const stored = await app.inject({ method: 'GET', url: '/readings/status' });
  assert.equal(JSON.parse(stored.body).totals.readings, 0);
});

test('an unauthorized POST is not recorded in the ingest log', async () => {
  const app = testApp();
  await app.inject({
    method: 'POST',
    url: '/readings',
    headers: { authorization: 'Bearer wrong' },
    payload: [reading('nope', '09:00:00')],
  });
  const { ingestEvents } = JSON.parse((await app.inject({ method: 'GET', url: '/readings/status' })).body);
  assert.deepEqual(ingestEvents, []);
});
