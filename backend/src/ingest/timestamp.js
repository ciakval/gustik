// Naive timestamps (no Z, no offset) are interpreted as this IANA zone.
// Centralized here so backend interpretation and frontend display
// (backend/src/static/timezone.js) can be kept in sync by eye.
const NAIVE_INPUT_TIMEZONE = 'Europe/Prague';

const CAPTURED_AT_PATTERN =
  /^(\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2})(?:\.(\d{1,3}))?(Z|[+-]\d{2}:\d{2})?$/;

const pragueFormatter = new Intl.DateTimeFormat('en-US', {
  timeZone: NAIVE_INPUT_TIMEZONE,
  year: 'numeric',
  month: '2-digit',
  day: '2-digit',
  hour: '2-digit',
  minute: '2-digit',
  second: '2-digit',
  hourCycle: 'h23',
});

function partsToUtcMs(parts) {
  return Date.UTC(
    Number(parts.year),
    Number(parts.month) - 1,
    Number(parts.day),
    Number(parts.hour),
    Number(parts.minute),
    Number(parts.second),
  );
}

// How would `instantMs` be rendered as a Prague wall clock, reinterpreted
// as if that rendering were itself UTC? Used below to measure Prague's
// offset from UTC at a given instant.
function renderAsIfUtcMs(instantMs) {
  const parts = Object.fromEntries(
    pragueFormatter.formatToParts(new Date(instantMs)).map((part) => [part.type, part.value]),
  );
  return partsToUtcMs(parts);
}

// Given wall-clock components meant as Europe/Prague local time, find the
// UTC instant they refer to. First guess treats the components as if they
// were already UTC, then corrects by how far that guess's Prague rendering
// drifted from the target - converges in one correction outside the
// ~1hr/year DST transition window (spring-forward gap / fall-back overlap
// is a known, accepted ambiguity - see design doc, not specially handled).
function pragueWallClockToUtcMs(year, month, day, hour, minute, second, ms) {
  const guessMs = Date.UTC(year, month - 1, day, hour, minute, second, ms);
  const offsetMs = renderAsIfUtcMs(guessMs) - guessMs;
  return guessMs - offsetMs;
}

/**
 * Normalizes a captured-at timestamp to canonical UTC
 * (YYYY-MM-DDTHH:MM:SS.sssZ), or returns null if it doesn't parse.
 *
 * Accepts: explicit "Z", any explicit +/-HH:MM offset, or naive (no zone -
 * interpreted as Europe/Prague wall-clock time). Storage and every query
 * that reads captured_at back (SQLite ORDER BY/WHERE, the ingest layer's
 * backfill `<` comparison) rely on every stored value sharing this exact
 * canonical shape for their lexicographic comparisons to equal chronological
 * ones - see the design doc for why.
 */
export function normalizeCapturedAt(input) {
  if (typeof input !== 'string') return null;

  const match = CAPTURED_AT_PATTERN.exec(input);
  if (!match) return null;

  const [, year, month, day, hour, minute, second, fraction, zone] = match;
  const ms = fraction ? Number(fraction.padEnd(3, '0')) : 0;

  if (zone) {
    const date = new Date(input);
    return Number.isNaN(date.getTime()) ? null : date.toISOString();
  }

  const utcMs = pragueWallClockToUtcMs(
    Number(year),
    Number(month),
    Number(day),
    Number(hour),
    Number(minute),
    Number(second),
    ms,
  );
  const date = new Date(utcMs);
  return Number.isNaN(date.getTime()) ? null : date.toISOString();
}
