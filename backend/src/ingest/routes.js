import { insertReading, getLatestCapturedAt } from '../store/readings.js';
import { broadcastReading, broadcastHistoryChanged } from '../serve/routes.js';
import { normalizeCapturedAt } from './timestamp.js';

const MAX_INGEST_EVENTS = 100;

function isAuthorized(request, token) {
  const header = request.headers.authorization ?? '';
  return header === `Bearer ${token}`;
}

// Ring buffer for the diagnostics page - the same counts the response now
// carries, kept per-batch so the last 100 POSTs stay inspectable from
// /status.html without the caller having to have logged anything.
function recordIngestEvent(fastify, event) {
  fastify.ingestEvents.push(event);
  if (fastify.ingestEvents.length > MAX_INGEST_EVENTS) {
    fastify.ingestEvents.shift();
  }
}

function wireShape(reading) {
  return {
    capturedAt: reading.capturedAt,
    windSpeedMs: reading.windSpeedMs,
    windDirOctant: reading.windDirOctant,
    rssiDbm: reading.rssiDbm ?? null,
  };
}

/**
 * Ingest endpoint.
 *
 * Response contract - the caller can tell exactly what happened to its rows:
 *
 *   401  bad/missing token
 *   400  nothing was stored and the batch is at fault (empty, or an
 *        unparseable capturedAt) - the batch is rejected whole, never partly
 *   201  at least one row was stored
 *   200  the batch was understood and is already fully accounted for, but
 *        NOTHING new was stored - every clientId was already known
 *
 * That last case is the one this contract exists for. `client_id` is UNIQUE
 * and the insert is `ON CONFLICT DO NOTHING` (a deliberate Story 2.2
 * backfill-retry safety net), so a batch can be silently discarded in full.
 * The old response was an unconditional `201 {written: <received count>}`,
 * which is how bug-031 hid for hours: the device reported "sent=yes" every
 * cycle while the backend stored nothing, and the HTTP response could not
 * have told it otherwise.
 *
 * Duplicates stay 2xx on purpose: they are not a transport failure. The data
 * IS on the server, retrying would change nothing, and a non-2xx would make
 * the firmware re-buffer rows that are already safely stored (main.cpp only
 * clears its flash buffer on a 2xx). What changes is that it is no longer a
 * *201 Created* - nothing was created - and the body says so explicitly.
 */
export function registerIngestRoutes(fastify, { ingestToken }) {
  fastify.post('/readings', async (request, reply) => {
    if (!isAuthorized(request, ingestToken)) {
      return reply.code(401).send({ error: 'unauthorized' });
    }

    const rawReadings = Array.isArray(request.body) ? request.body : [request.body];

    // Both firmware call sites are guarded against this (main.cpp sends
    // either exactly one reading or a buffer it has just checked is
    // non-empty), so an empty batch means a caller bug - answering 2xx
    // would hide it.
    if (rawReadings.length === 0) {
      return reply.code(400).send({ error: 'empty batch: send at least one reading' });
    }

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
    let insertedCount = 0;
    let backfilledCount = 0;
    for (const reading of readings) {
      const backfilled = latestCapturedAtBeforeBatch !== null && reading.capturedAt < latestCapturedAtBeforeBatch;
      const { inserted } = insertReading(fastify.db, reading, { backfilled });
      if (inserted) {
        insertedCount += 1;
        if (backfilled) {
          backfilledCount += 1;
          anyBackfilled = true;
        } else {
          broadcastReading(fastify, wireShape(reading));
        }
      }
    }
    if (anyBackfilled) {
      broadcastHistoryChanged(fastify);
    }

    const duplicates = readings.length - insertedCount;

    recordIngestEvent(fastify, {
      atIso: new Date().toISOString(),
      count: readings.length,
      inserted: insertedCount,
      duplicates,
      backfilled: backfilledCount,
      remoteAddress: request.ip,
    });

    // No `written` field: it counted what arrived, not what was stored, and
    // that conflation is the whole reason bug-031 was invisible. The four
    // counts below always satisfy received == inserted + duplicates.
    const body = { received: readings.length, inserted: insertedCount, duplicates, backfilled: backfilledCount };
    if (insertedCount === 0) {
      return reply.code(200).send({
        ...body,
        warning: 'no readings stored: every clientId in this batch was already known',
      });
    }
    return reply.code(201).send(body);
  });
}
