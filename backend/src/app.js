import Fastify from 'fastify';
import { openDb } from './store/db.js';
import { registerIngestRoutes } from './ingest/routes.js';
import { registerHealthRoutes } from './health/routes.js';

export function buildApp({ dbPath, ingestToken }) {
  const fastify = Fastify({ logger: false });
  const db = fastify.decorate('db', openDb(dbPath)).db;

  registerHealthRoutes(fastify);
  registerIngestRoutes(fastify, { ingestToken });

  return fastify;
}
