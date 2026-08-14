// Pure time-series aggregation for GET /readings/history. No DB, no HTTP -
// testable on its own (test/aggregate.test.js).
//
// Why this exists: the firmware samples every ~3s, so a full race day is
// ~28 800 rows. Rendering that raw into one canvas is both slow and
// unreadable. Bucketing collapses it to a fixed number of points while
// keeping the one thing that must survive averaging: the gust peak.

// "Nice" bucket widths, ascending. Kept in sync with static/timerange.js -
// the client picks from the same ladder so an auto-chosen bucket and a
// server-clamped one always land on the same round numbers (and therefore
// round tick labels). 3s is the floor: it is the firmware's own sample
// interval, so a finer bucket cannot add information.
export const BUCKET_LADDER_SECONDS = [3, 5, 10, 15, 30, 60, 120, 300, 600, 900, 1800, 3600, 7200];

const DEGREES_PER_OCTANT = 45;

/**
 * Circular (vector) mean of a list of octants, snapped back to an octant.
 *
 * Direction is an angle, not a number: octants 7 (SZ) and 1 (SV) straddle
 * north, and their arithmetic mean is 4 (J) - the exact opposite of the
 * truth. Averaging unit vectors and converting back is the only correct way.
 *
 * Snapping the result back to a whole octant is deliberate, not a rounding
 * convenience: NFR-6/AD-5 forbid ever surfacing a direction finer than the
 * vane's 8 physical steps.
 *
 * Returns null for an empty list. When the samples cancel out exactly
 * (e.g. [2, 6] - due east and due west), the mean vector is zero and has no
 * direction at all; reporting the first sample is a visible, arbitrary-but-
 * honest choice, whereas atan2(0, 0) would silently claim "north".
 */
export function circularMeanOctant(octants) {
  if (octants.length === 0) {
    return null;
  }
  let east = 0;
  let north = 0;
  for (const octant of octants) {
    const radians = (octant * DEGREES_PER_OCTANT * Math.PI) / 180;
    east += Math.sin(radians);
    north += Math.cos(radians);
  }
  if (Math.abs(east) < 1e-9 && Math.abs(north) < 1e-9) {
    return octants[0];
  }
  const degrees = (Math.atan2(east, north) * 180) / Math.PI;
  const normalized = (degrees + 360) % 360;
  return Math.round(normalized / DEGREES_PER_OCTANT) % 8;
}

function bucketStartIso(capturedAt, bucketSeconds) {
  const ms = Date.parse(capturedAt);
  const bucketMs = bucketSeconds * 1000;
  return new Date(Math.floor(ms / bucketMs) * bucketMs).toISOString();
}

function summarize(startIso, samples) {
  const speeds = samples.map((r) => r.windSpeedMs);
  const rssis = samples.map((r) => r.rssiDbm).filter((v) => v !== null && v !== undefined);
  return {
    capturedAt: startIso,
    windSpeedMs: speeds.reduce((a, b) => a + b, 0) / speeds.length,
    windSpeedMinMs: Math.min(...speeds),
    windSpeedMaxMs: Math.max(...speeds),
    windDirOctant: circularMeanOctant(samples.map((r) => r.windDirOctant)),
    // null (not 0) when nothing in this bucket ever reported RSSI - the
    // firmware sends null before its first successful Wi-Fi scan.
    rssiDbm: rssis.length === 0 ? null : rssis.reduce((a, b) => a + b, 0) / rssis.length,
    sampleCount: samples.length,
  };
}

/**
 * Collapse ascending-by-capturedAt readings into fixed-width buckets.
 *
 * Buckets are aligned to the epoch (not to the first sample) so the same
 * window always produces the same bucket boundaries regardless of when it
 * was requested - a redraw after new data arrives doesn't shift every point.
 *
 * Empty buckets are OMITTED, never zero-filled: a gap in the data must look
 * like a gap on the chart, not like a sudden calm. `sampleCount` lets the
 * client tell a thin bucket from a full one.
 *
 * bucketSeconds of 0/null/undefined means "no aggregation" - the input is
 * returned as-is, which is the shipped raw-rows contract.
 */
export function bucketReadings(readings, bucketSeconds) {
  if (!bucketSeconds || bucketSeconds <= 0) {
    return readings;
  }
  const buckets = [];
  let currentStart = null;
  let currentSamples = [];

  for (const reading of readings) {
    const start = bucketStartIso(reading.capturedAt, bucketSeconds);
    if (start !== currentStart) {
      if (currentStart !== null) {
        buckets.push(summarize(currentStart, currentSamples));
      }
      currentStart = start;
      currentSamples = [];
    }
    currentSamples.push(reading);
  }
  if (currentStart !== null) {
    buckets.push(summarize(currentStart, currentSamples));
  }
  return buckets;
}

/**
 * Find stretches where the station stopped reporting.
 *
 * The sample interval is a firmware constant we deliberately do NOT hardcode
 * here (it is a placeholder in main.cpp and will change): the median gap
 * between consecutive readings IS the interval, measured from the data
 * itself. Anything more than `multiple`x that - and at least `floorSeconds`,
 * so a momentarily jittery 3s cadence doesn't spam the log - is an outage.
 *
 * Returns [] for fewer than 3 readings: no median, nothing to compare against.
 */
export function detectGaps(readings, { multiple = 3, floorSeconds = 15 } = {}) {
  if (readings.length < 3) {
    return [];
  }
  const deltas = [];
  for (let i = 1; i < readings.length; i += 1) {
    deltas.push((Date.parse(readings[i].capturedAt) - Date.parse(readings[i - 1].capturedAt)) / 1000);
  }
  const sorted = [...deltas].sort((a, b) => a - b);
  const median = sorted[Math.floor(sorted.length / 2)];
  const threshold = Math.max(median * multiple, floorSeconds);

  const gaps = [];
  for (let i = 0; i < deltas.length; i += 1) {
    if (deltas[i] > threshold) {
      gaps.push({
        fromIso: readings[i].capturedAt,
        toIso: readings[i + 1].capturedAt,
        seconds: Math.round(deltas[i]),
      });
    }
  }
  return gaps;
}

/**
 * Raise `bucketSeconds` until the requested range fits within `maxPoints`.
 * Guards the response size against a client asking for 3s resolution over a
 * whole day. Snaps to BUCKET_LADDER_SECONDS so the result stays a round
 * number; falls back to the exact required width past the end of the ladder.
 */
export function clampBucketSeconds(bucketSeconds, rangeSeconds, maxPoints = 2000) {
  const required = rangeSeconds / maxPoints;
  if (bucketSeconds >= required) {
    return bucketSeconds;
  }
  return BUCKET_LADDER_SECONDS.find((candidate) => candidate >= required) ?? Math.ceil(required);
}
