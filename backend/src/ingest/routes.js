import { insertReading } from '../store/readings.js';
import { broadcastReading } from '../serve/routes.js';

function isAuthorized(request, token) {
  const header = request.headers.authorization ?? '';
  return header === `Bearer ${token}`;
}

function wireShape(reading) {
  return {
    capturedAt: reading.capturedAt,
    windSpeedMs: reading.windSpeedMs,
    windDirOctant: reading.windDirOctant,
    rssiDbm: reading.rssiDbm ?? null,
  };
}

export function registerIngestRoutes(fastify, { ingestToken }) {
  fastify.post('/readings', async (request, reply) => {
    if (!isAuthorized(request, ingestToken)) {
      return reply.code(401).send({ error: 'unauthorized' });
    }

    const readings = Array.isArray(request.body) ? request.body : [request.body];
    for (const reading of readings) {
      const { inserted } = insertReading(fastify.db, reading);
      if (inserted) {
        broadcastReading(fastify, wireShape(reading));
      }
    }

    return reply.code(201).send({ written: readings.length });
  });
}
