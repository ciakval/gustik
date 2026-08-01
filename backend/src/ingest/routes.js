import { insertReading } from '../store/readings.js';

function isAuthorized(request, token) {
  const header = request.headers.authorization ?? '';
  return header === `Bearer ${token}`;
}

export function registerIngestRoutes(fastify, { ingestToken }) {
  fastify.post('/readings', async (request, reply) => {
    if (!isAuthorized(request, ingestToken)) {
      return reply.code(401).send({ error: 'unauthorized' });
    }

    const readings = Array.isArray(request.body) ? request.body : [request.body];
    for (const reading of readings) {
      insertReading(fastify.db, reading);
    }

    return reply.code(201).send({ written: readings.length });
  });
}
