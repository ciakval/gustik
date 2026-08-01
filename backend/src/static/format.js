const KNOTS_PER_MS = 1.9438444924406;
const STALE_THRESHOLD_MS = 2 * 60 * 1000; // NFR-3, independent of the LED's 10s threshold (NFR-2)

const OCTANT_LABELS = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW'];

export function msToKnots(ms) {
  return ms * KNOTS_PER_MS;
}

export function octantToCompassLabel(octant) {
  return OCTANT_LABELS[octant];
}

export function isStale(capturedAtIso, nowIso = new Date().toISOString()) {
  const ageMs = new Date(nowIso).getTime() - new Date(capturedAtIso).getTime();
  return ageMs >= STALE_THRESHOLD_MS;
}

export function formatAge(capturedAtIso, nowIso = new Date().toISOString()) {
  const ageSeconds = Math.floor((new Date(nowIso).getTime() - new Date(capturedAtIso).getTime()) / 1000);
  if (ageSeconds < 60) {
    return `${ageSeconds}s`;
  }
  return `${Math.floor(ageSeconds / 60)}min`;
}
