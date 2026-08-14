import {
  getLatest,
  getHistory,
  getHistoryFull,
  getLatestFull,
  getRecentFull,
  getTotals,
} from '../store/readings.js';
import { bucketReadings, clampBucketSeconds, detectGaps } from './aggregate.js';
import { startOfLocalDayIso } from '../store/localday.js';

const MAX_BUCKETS = 2000;
const MAX_RECENT = 200;
const DEFAULT_RECENT = 30;

// Every stored captured_at is exactly 'YYYY-MM-DDTHH:MM:SS.sssZ' (enforced by
// the ingest route) and the history query compares it as TEXT, so any
// caller-supplied bound has to be normalized to that same shape - a bound
// like '2026-08-14T10:00Z' would compare wrong lexicographically without
// erroring anywhere. Returns null for anything unparseable.
function toCanonicalIso(value) {
  if (value === undefined || value === null || value === '') {
    return undefined;
  }
  const ms = Date.parse(value);
  return Number.isNaN(ms) ? null : new Date(ms).toISOString();
}

function parseWindow(query, now) {
  const from = toCanonicalIso(query.from);
  const to = toCanonicalIso(query.to);
  if (from === null || to === null) {
    return { error: 'invalid from/to: must be ISO-8601, e.g. 2026-08-14T09:00:00.000Z' };
  }
  return {
    from: from ?? startOfLocalDayIso(now),
    to: to ?? now.toISOString(),
  };
}

function parseBucketSeconds(raw, from, to) {
  if (raw === undefined || raw === '' || raw === 'auto') {
    return { bucketSeconds: 0 };
  }
  const requested = Number(raw);
  if (!Number.isFinite(requested) || requested < 0) {
    return { error: 'invalid bucket: must be a non-negative number of seconds' };
  }
  if (requested === 0) {
    return { bucketSeconds: 0 };
  }
  const rangeSeconds = (Date.parse(to) - Date.parse(from)) / 1000;
  return { bucketSeconds: clampBucketSeconds(requested, Math.max(rangeSeconds, 0), MAX_BUCKETS) };
}

export function registerServeRoutes(fastify) {
  fastify.get('/readings/latest', async () => {
    return { reading: getLatest(fastify.db) };
  });

  // Never accepts a unit parameter (Consistency Conventions) - always SI on
  // the wire, unit conversion is client-only (FR-10).
  //
  // Optional query params, all backwards compatible (no params == the
  // originally shipped behaviour, apart from the day boundary now being
  // Europe/Prague local midnight instead of 00:00 UTC):
  //   from, to  - ISO-8601 window bounds, inclusive
  //   bucket    - aggregation width in seconds; omitted/0/'auto' = raw rows
  //
  // The response echoes the window and the *effective* bucket, which may be
  // coarser than requested (MAX_BUCKETS clamp) - the client shows the
  // effective value so a silently-downgraded resolution is never invisible.
  fastify.get('/readings/history', async (request, reply) => {
    const now = new Date();
    const window = parseWindow(request.query, now);
    if (window.error) {
      return reply.code(400).send({ error: window.error });
    }
    const { bucketSeconds, error } = parseBucketSeconds(request.query.bucket, window.from, window.to);
    if (error) {
      return reply.code(400).send({ error });
    }

    const raw = getHistory(fastify.db, { now, from: window.from, to: window.to });
    return {
      readings: bucketReadings(raw, bucketSeconds),
      from: window.from,
      to: window.to,
      bucketSeconds,
    };
  });

  // Diagnostics for the /status.html page. Public and unauthenticated, same
  // as everything else here (FR-9) - it exposes no data the other read
  // endpoints don't already serve, just more fields of it.
  fastify.get('/readings/status', async (request, reply) => {
    const now = new Date();
    const window = parseWindow(request.query, now);
    if (window.error) {
      return reply.code(400).send({ error: window.error });
    }
    const limit = Math.min(Number(request.query.limit) || DEFAULT_RECENT, MAX_RECENT);

    return {
      serverTimeIso: now.toISOString(),
      latest: getLatestFull(fastify.db),
      totals: getTotals(fastify.db),
      recent: getRecentFull(fastify.db, limit),
      // In-memory, resets on restart by design (see the design doc) - it is
      // the one thing the DB cannot reconstruct: a POST whose rows all
      // collided on client_id leaves no trace in the table at all. That is
      // precisely how bug-031 stayed hidden.
      ingestEvents: [...fastify.ingestEvents].reverse(),
      gaps: detectGaps(getHistoryFull(fastify.db, { now, from: window.from, to: window.to })),
      from: window.from,
      to: window.to,
    };
  });

  fastify.get('/readings/live', { websocket: true }, (socket) => {
    fastify.wsClients.add(socket);
    socket.on('close', () => fastify.wsClients.delete(socket));
  });
}

function broadcast(fastify, message) {
  const payload = JSON.stringify(message);
  for (const socket of fastify.wsClients) {
    if (socket.readyState === socket.OPEN) {
      socket.send(payload);
    }
  }
}

export function broadcastReading(fastify, reading) {
  broadcast(fastify, reading);
}

// Bare event (AD-9) - tells listeners to refetch /readings/history rather
// than carrying any payload itself. Harmless if nothing is listening yet
// (Story 2.3 AC3 - Epic 3 is the first real consumer).
export function broadcastHistoryChanged(fastify) {
  broadcast(fastify, { event: 'history-changed' });
}
