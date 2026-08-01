import Fastify from 'fastify';
import fastifyWebsocket from '@fastify/websocket';
import { openDb } from './store/db.js';
import { registerIngestRoutes } from './ingest/routes.js';
import { registerHealthRoutes } from './health/routes.js';
import { registerServeRoutes } from './serve/routes.js';

export function buildApp({ dbPath, ingestToken }) {
  const fastify = Fastify({ logger: false });
  fastify.decorate('db', openDb(dbPath));
  fastify.decorate('wsClients', new Set());
  fastify.register(fastifyWebsocket);

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
