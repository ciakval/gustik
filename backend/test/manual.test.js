import { test } from 'node:test';
import assert from 'node:assert/strict';
import { buildApp } from '../src/app.js';

function testApp() {
  return buildApp({ dbPath: ':memory:', ingestToken: 'secret-token' });
}

test('GET /manual.html serves the operation manual without any auth (FR-15/FR-9-style public access)', async () => {
  const app = testApp();
  const res = await app.inject({ method: 'GET', url: '/manual.html' });
  assert.equal(res.statusCode, 200);
  assert.match(res.headers['content-type'], /text\/html/);
});

test('the manual covers powering on, LED meaning, dashboard freshness check, and a failure procedure (AC1)', async () => {
  const app = testApp();
  const res = await app.inject({ method: 'GET', url: '/manual.html' });
  const body = res.body.toLowerCase();
  assert.match(body, /powerbank/); // power-on step
  assert.match(body, /led/); // LED meaning
  assert.match(body, /dashboard|živá data|živé hodnoty/); // dashboard freshness check
  assert.match(body, /nefunguje|selhání|problém/); // failure procedure section
});

test('the dashboard links to the manual', async () => {
  const app = testApp();
  const res = await app.inject({ method: 'GET', url: '/' });
  assert.match(res.body, /manual\.html/);
});
