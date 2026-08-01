import { insertReading, getLatestCapturedAt } from '../store/readings.js';
import { broadcastReading, broadcastHistoryChanged } from '../serve/routes.js';

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
    // Snapshot once per request (AD-9: "older than the last SO FAR
    // received" at the moment the batch arrives) - every record in this
    // batch is compared against the same baseline, not against each other.
    const latestCapturedAtBeforeBatch = getLatestCapturedAt(fastify.db);

    let anyBackfilled = false;
    for (const reading of readings) {
      const backfilled = latestCapturedAtBeforeBatch !== null && reading.capturedAt < latestCapturedAtBeforeBatch;
      const { inserted } = insertReading(fastify.db, reading, { backfilled });
      if (inserted) {
        if (backfilled) {
          anyBackfilled = true;
        } else {
          broadcastReading(fastify, wireShape(reading));
        }
      }
    }
    if (anyBackfilled) {
      broadcastHistoryChanged(fastify);
    }

    return reply.code(201).send({ written: readings.length });
  });
}
