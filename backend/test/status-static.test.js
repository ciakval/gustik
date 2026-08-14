import { test } from 'node:test';
import assert from 'node:assert/strict';
import { buildApp } from '../src/app.js';

function testApp() {
  return buildApp({ dbPath: ':memory:', ingestToken: 'secret-token' });
}

test('GET /status.html serves the diagnostics page without auth', async () => {
  const app = testApp();
  const res = await app.inject({ method: 'GET', url: '/status.html' });
  assert.equal(res.statusCode, 200);
  assert.match(res.headers['content-type'], /text\/html/);
  assert.match(res.body, /stav stanice/i);
});

test('the dashboard links to the status page and the manual', async () => {
  const app = testApp();
  const res = await app.inject({ method: 'GET', url: '/' });
  assert.match(res.body, /href="\/status\.html"/);
  assert.match(res.body, /href="\/manual\.html"/);
});

test('every ES module the two pages load is actually served', async () => {
  const app = testApp();
  const modules = [
    '/dashboard.js',
    '/status.js',
    '/compass.js',
    '/beaufort.js',
    '/timerange.js',
    '/timerange-ui.js',
    '/history-chart.js',
    '/history-chart-data.js',
    '/live-socket.js',
    '/format.js',
    '/timezone.js',
    '/styles.css',
  ];
  for (const path of modules) {
    const res = await app.inject({ method: 'GET', url: path });
    assert.equal(res.statusCode, 200, `${path} should be served`);
  }
});
