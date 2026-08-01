import { test } from 'node:test';
import assert from 'node:assert/strict';
import { buildApp } from '../src/app.js';

test('GET /health returns 200 when the SQLite file can be opened', async () => {
  const app = buildApp({ dbPath: ':memory:', ingestToken: 'secret-token' });
  const res = await app.inject({ method: 'GET', url: '/health' });
  assert.equal(res.statusCode, 200);
});

test('GET /health requires no auth token', async () => {
  const app = buildApp({ dbPath: ':memory:', ingestToken: 'secret-token' });
  const res = await app.inject({ method: 'GET', url: '/health' });
  assert.equal(res.statusCode, 200);
});
