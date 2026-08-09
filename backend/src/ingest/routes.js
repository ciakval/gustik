import { insertReading, getLatestCapturedAt } from '../store/readings.js';
import { broadcastReading, broadcastHistoryChanged } from '../serve/routes.js';
import { normalizeCapturedAt } from './timestamp.js';

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

    const rawReadings = Array.isArray(request.body) ? request.body : [request.body];

    // getHistory (readings.js) filters by lexicographically comparing
    // captured_at against a generated 'YYYY-MM-DDTHH:MM:SS.sssZ' cutoff -
    // any other shape silently sorts wrong and drops the row from history
    // without erroring anywhere. normalizeCapturedAt() converts every
    // accepted shape (explicit "Z", explicit offset, or naive-as-Prague-
    // local) to that one canonical shape; readings whose capturedAt can't
    // be parsed at all become null here and 400 the whole batch below.
    const readings = rawReadings.map((reading) => ({
      ...reading,
      capturedAt: normalizeCapturedAt(reading.capturedAt),
    }));

    const invalid = readings.find((reading) => reading.capturedAt === null);
    if (invalid) {
      return reply.code(400).send({ error: 'invalid capturedAt: must be ISO-8601, e.g. 2026-08-01T09:00:00.000Z or 2026-08-01T11:00:00+02:00' });
    }

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
