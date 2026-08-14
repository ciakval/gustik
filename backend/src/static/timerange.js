// Time-range and resolution model for the history charts. Pure - no DOM,
// unit tested. The UI that renders it lives in timerange-ui.js.
//
// Shape borrowed from Grafana: pick a window, and the resolution follows
// automatically unless you override it. The effective resolution is always
// displayed, so an override is never silently in effect.

// Kept in sync with the backend's serve/aggregate.js BUCKET_LADDER_SECONDS.
// Round numbers only, so time-axis ticks land on round numbers too. 3s is the
// floor: it is the firmware's own sample interval, so a finer bucket cannot
// add information.
export const BUCKET_LADDER_SECONDS = [3, 5, 10, 15, 30, 60, 120, 300, 600, 900, 1800, 3600, 7200];

export const RANGES = [
  { id: '15m', label: '15 min', seconds: 15 * 60 },
  { id: '1h', label: '1 h', seconds: 60 * 60 },
  { id: '3h', label: '3 h', seconds: 3 * 60 * 60 },
  { id: '6h', label: '6 h', seconds: 6 * 60 * 60 },
  { id: '12h', label: '12 h', seconds: 12 * 60 * 60 },
  // Not a fixed duration - anchored to local midnight, resolved in windowFor().
  { id: 'today', label: 'dnes', seconds: null },
];

export const DEFAULT_RANGE_ID = '1h';

const STORAGE_RANGE_KEY = 'gustik.range';
const STORAGE_BUCKET_KEY = 'gustik.bucket';

export function rangeById(id) {
  return RANGES.find((range) => range.id === id) ?? RANGES.find((range) => range.id === DEFAULT_RANGE_ID);
}

/**
 * Bucket width that lands close to `targetPoints` samples across the window,
 * snapped up to the next entry on the ladder. Snapping up (never down) is
 * what guarantees the point count stays at or below the target.
 */
export function autoBucketSeconds(rangeSeconds, targetPoints = 180) {
  const ideal = rangeSeconds / targetPoints;
  return BUCKET_LADDER_SECONDS.find((candidate) => candidate >= ideal) ?? BUCKET_LADDER_SECONDS.at(-1);
}

/** Ladder entries worth offering for a window: at least ~6 points, at most one per 2s of raw data. */
export function bucketOptionsFor(rangeSeconds) {
  return BUCKET_LADDER_SECONDS.filter((candidate) => rangeSeconds / candidate >= 6);
}

export function formatBucketSeconds(seconds) {
  if (seconds < 60) return `${seconds} s`;
  if (seconds < 3600) return `${seconds / 60} min`;
  return `${seconds / 3600} h`;
}

/**
 * Resolve a selection into the concrete {from, to, bucketSeconds} a request
 * needs. `startOfDayMs` is injected rather than computed here so the caller
 * decides the timezone (the dashboard uses Europe/Prague via timezone.js) and
 * so this stays testable without a clock.
 */
export function windowFor({ rangeId, bucketSeconds = 'auto', nowMs = Date.now(), startOfDayMs }) {
  const range = rangeById(rangeId);
  const fromMs = range.seconds === null ? startOfDayMs : nowMs - range.seconds * 1000;
  const rangeSeconds = Math.max((nowMs - fromMs) / 1000, 1);
  const effectiveBucket =
    bucketSeconds === 'auto' || !bucketSeconds ? autoBucketSeconds(rangeSeconds) : Number(bucketSeconds);
  return {
    from: new Date(fromMs).toISOString(),
    to: new Date(nowMs).toISOString(),
    rangeSeconds,
    bucketSeconds: effectiveBucket,
  };
}

// Persisted so a phone reopening the page on the boat keeps its view.
// Storage can throw (private mode, disabled cookies) - a failed preference
// must never take the dashboard down with it.
export function loadSelection(storage = globalThis.localStorage) {
  try {
    return {
      rangeId: storage?.getItem(STORAGE_RANGE_KEY) ?? DEFAULT_RANGE_ID,
      bucketSeconds: storage?.getItem(STORAGE_BUCKET_KEY) ?? 'auto',
    };
  } catch {
    return { rangeId: DEFAULT_RANGE_ID, bucketSeconds: 'auto' };
  }
}

export function saveSelection({ rangeId, bucketSeconds }, storage = globalThis.localStorage) {
  try {
    storage?.setItem(STORAGE_RANGE_KEY, rangeId);
    storage?.setItem(STORAGE_BUCKET_KEY, String(bucketSeconds));
  } catch {
    // Preference is a nice-to-have; losing it is not worth an error.
  }
}
