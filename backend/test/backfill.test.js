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

function getRawRow(app, clientId) {
  return app.db.prepare('SELECT * FROM readings WHERE client_id = ?').get(clientId);
}

test('a live-order record (newer than the current latest) is stored with backfilled=0', async () => {
  const app = testApp();
  await postReadings(app, [
    { clientId: 'live-1', capturedAt: '2026-08-01T09:00:00.000Z', clockSynced: true, windSpeedMs: 1, windDirOctant: 0, rssiDbm: -50 },
  ]);
  const row = getRawRow(app, 'live-1');
  assert.equal(row.backfilled, 0);
});

test('a record older than the current latest is stored with backfilled=1 (AC1)', async () => {
  const app = testApp();
  await postReadings(app, [
    { clientId: 'live-1', capturedAt: '2026-08-01T09:10:00.000Z', clockSynced: true, windSpeedMs: 1, windDirOctant: 0, rssiDbm: -50 },
  ]);
  await postReadings(app, [
    { clientId: 'backfilled-1', capturedAt: '2026-08-01T09:00:00.000Z', clockSynced: false, windSpeedMs: 2, windDirOctant: 1, rssiDbm: -60 },
  ]);

  const row = getRawRow(app, 'backfilled-1');
  assert.equal(row.backfilled, 1);
});

test('a backfill batch (multiple older records) broadcasts a bare history-changed WS event (AC2)', async () => {
  const app = testApp();
  await postReadings(app, [
    { clientId: 'live-1', capturedAt: '2026-08-01T09:10:00.000Z', clockSynced: true, windSpeedMs: 1, windDirOctant: 0, rssiDbm: -50 },
  ]);

  await app.listen({ port: 0, host: '127.0.0.1' });
  const { port } = app.server.address();
  let ws;
  try {
    ws = new WebSocket(`ws://127.0.0.1:${port}/readings/live`);
    await new Promise((resolve, reject) => {
      ws.addEventListener('open', resolve);
      ws.addEventListener('error', () => reject(new Error('ws connect failed')));
    });

    const historyChanged = new Promise((resolve) => {
      ws.addEventListener('message', (event) => {
        const msg = JSON.parse(event.data);
        if (msg.event === 'history-changed') {
          resolve(msg);
        }
      });
    });

    await postReadings(app, [
      { clientId: 'backfilled-1', capturedAt: '2026-08-01T09:00:00.000Z', clockSynced: false, windSpeedMs: 2, windDirOctant: 1, rssiDbm: -60 },
      { clientId: 'backfilled-2', capturedAt: '2026-08-01T09:05:00.000Z', clockSynced: false, windSpeedMs: 3, windDirOctant: 2, rssiDbm: -61 },
    ]);

    const msg = await Promise.race([
      historyChanged,
      new Promise((_, reject) => setTimeout(() => reject(new Error('timed out waiting for history-changed')), 2000)),
    ]);
    assert.deepEqual(msg, { event: 'history-changed' });
  } finally {
    ws?.close();
    await app.close();
  }
});

test('a live-only batch never broadcasts history-changed (no false positives)', async () => {
  const app = testApp();
  await app.listen({ port: 0, host: '127.0.0.1' });
  const { port } = app.server.address();
  let ws;
  try {
    ws = new WebSocket(`ws://127.0.0.1:${port}/readings/live`);
    await new Promise((resolve, reject) => {
      ws.addEventListener('open', resolve);
      ws.addEventListener('error', () => reject(new Error('ws connect failed')));
    });

    let historyChangedSeen = false;
    ws.addEventListener('message', (event) => {
      const msg = JSON.parse(event.data);
      if (msg.event === 'history-changed') {
        historyChangedSeen = true;
      }
    });

    await postReadings(app, [
      { clientId: 'live-1', capturedAt: '2026-08-01T09:00:00.000Z', clockSynced: true, windSpeedMs: 1, windDirOctant: 0, rssiDbm: -50 },
    ]);
    // give any (wrongly fired) broadcast time to arrive
    await new Promise((resolve) => setTimeout(resolve, 200));
    assert.equal(historyChangedSeen, false);
  } finally {
    ws?.close();
    await app.close();
  }
});

test('a backfilled batch with no listeners does not error the request (AC3)', async () => {
  const app = testApp();
  await postReadings(app, [
    { clientId: 'live-1', capturedAt: '2026-08-01T09:10:00.000Z', clockSynced: true, windSpeedMs: 1, windDirOctant: 0, rssiDbm: -50 },
  ]);
  const res = await postReadings(app, [
    { clientId: 'backfilled-1', capturedAt: '2026-08-01T09:00:00.000Z', clockSynced: false, windSpeedMs: 2, windDirOctant: 1, rssiDbm: -60 },
  ]);
  assert.ok(res.statusCode >= 200 && res.statusCode < 300);
});
