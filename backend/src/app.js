import path from 'node:path';
import { fileURLToPath } from 'node:url';
import Fastify from 'fastify';
import fastifyWebsocket from '@fastify/websocket';
import fastifyStatic from '@fastify/static';
import { openDb } from './store/db.js';
import { registerIngestRoutes } from './ingest/routes.js';
import { registerHealthRoutes } from './health/routes.js';
import { registerServeRoutes } from './serve/routes.js';

const staticRoot = path.join(path.dirname(fileURLToPath(import.meta.url)), 'static');

export function buildApp({ dbPath, ingestToken }) {
  const fastify = Fastify({ logger: false });
  fastify.decorate('db', openDb(dbPath));
  fastify.decorate('wsClients', new Set());
  // Bounded in-memory log of recent POST /readings calls, oldest first, for
  // the diagnostics page. Deliberately not persisted: it is a "what is the
  // device doing right now" view, not a record.
  fastify.decorate('ingestEvents', []);
  fastify.register(fastifyWebsocket);
  fastify.register(fastifyStatic, { root: staticRoot });

  // routes must be declared in a child plugin context registered AFTER
  // @fastify/websocket, so its onRoute hook is live before routes attach
  // (declaring them synchronously in the same tick as an unawaited
  // register() attaches them too early - socket.send is then undefined)
  fastify.register(async (instance) => {
    registerHealthRoutes(instance);
    registerServeRoutes(instance);
    registerIngestRoutes(instance, { ingestToken });
  });

  return fastify;
}
