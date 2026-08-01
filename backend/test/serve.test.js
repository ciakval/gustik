import { test } from 'node:test';
import assert from 'node:assert/strict';
import { WebSocket } from 'ws';
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

test('GET /readings/latest returns a clear "no data" response when the table is empty', async () => {
  const app = testApp();
  const res = await app.inject({ method: 'GET', url: '/readings/latest' });
  assert.equal(res.statusCode, 200);
  assert.deepEqual(JSON.parse(res.body), { reading: null });
});

test('GET /readings/latest returns the newest record by capturedAt, in camelCase shape', async () => {
  const app = testApp();
  await postReadings(app, [
    { clientId: 'a', capturedAt: '2026-08-01T09:05:00.000Z', clockSynced: true, windSpeedMs: 5, windDirOctant: 1, rssiDbm: -50 },
  ]);
  // inserted second (higher insertion order) but earlier captured_at - must NOT win
  await postReadings(app, [
    { clientId: 'b', capturedAt: '2026-08-01T09:00:00.000Z', clockSynced: true, windSpeedMs: 1, windDirOctant: 0, rssiDbm: -60 },
  ]);

  const res = await app.inject({ method: 'GET', url: '/readings/latest' });
  assert.equal(res.statusCode, 200);
  const { reading } = JSON.parse(res.body);
  assert.deepEqual(reading, {
    capturedAt: '2026-08-01T09:05:00.000Z',
    windSpeedMs: 5,
    windDirOctant: 1,
    rssiDbm: -50,
  });
});

function withTimeout(promise, ms, label) {
  return Promise.race([
    promise,
    new Promise((_, reject) => setTimeout(() => reject(new Error(`timed out: ${label}`)), ms)),
  ]);
}

test('a successful POST /readings write broadcasts an identically-shaped message on the WS channel', async () => {
  const app = testApp();
  await app.listen({ port: 0, host: '127.0.0.1' });
  const { port } = app.server.address();
  let ws;

  try {
    ws = new WebSocket(`ws://127.0.0.1:${port}/readings/live`);
    const opened = new Promise((resolve, reject) => {
      ws.addEventListener('open', resolve);
      ws.addEventListener('error', () => reject(new Error('ws connect failed')));
    });
    await withTimeout(opened, 2000, 'ws open');

    const received = new Promise((resolve) => {
      ws.addEventListener('message', (event) => resolve(JSON.parse(event.data)));
    });

    await postReadings(app, [
      { clientId: 'ws-1', capturedAt: '2026-08-01T09:00:00.000Z', clockSynced: true, windSpeedMs: 2.5, windDirOctant: 4, rssiDbm: -70 },
    ]);

    const message = await withTimeout(received, 2000, 'ws message');
    assert.deepEqual(message, {
      capturedAt: '2026-08-01T09:00:00.000Z',
      windSpeedMs: 2.5,
      windDirOctant: 4,
      rssiDbm: -70,
    });
  } finally {
    ws?.close();
    await app.close();
  }
});
