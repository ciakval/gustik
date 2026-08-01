import { test } from 'node:test';
import assert from 'node:assert/strict';
import { buildApp } from '../src/app.js';

function testApp() {
  return buildApp({ dbPath: ':memory:', ingestToken: 'secret-token' });
}

test('GET / serves the dashboard HTML without any auth (FR-9)', async () => {
  const app = testApp();
  const res = await app.inject({ method: 'GET', url: '/' });
  assert.equal(res.statusCode, 200);
  assert.match(res.headers['content-type'], /text\/html/);
  assert.match(res.body, /<title>/i);
});

test('GET /format.js serves the static JS module used by the dashboard', async () => {
  const app = testApp();
  const res = await app.inject({ method: 'GET', url: '/format.js' });
  assert.equal(res.statusCode, 200);
});

test('GET /vendor/chart.umd.min.js serves the vendored Chart.js bundle (no CDN dependency)', async () => {
  const app = testApp();
  const res = await app.inject({ method: 'GET', url: '/vendor/chart.umd.min.js' });
  assert.equal(res.statusCode, 200);
  assert.match(res.body, /Chart\.js/);
});
