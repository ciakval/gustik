// Single named seam for the dashboard's display timezone. A future
// per-user-preference override (e.g. read from localStorage) only needs to
// change what this resolves to - see CLAUDE.md "Choosing a workflow" /
// the timestamp-timezone-support design doc.
export const DISPLAY_TIMEZONE = 'Europe/Prague';

function formatter(options) {
  return new Intl.DateTimeFormat('cs-CZ', { timeZone: DISPLAY_TIMEZONE, hourCycle: 'h23', ...options });
}

const hhmm = formatter({ hour: '2-digit', minute: '2-digit' });
const hhmmss = formatter({ hour: '2-digit', minute: '2-digit', second: '2-digit' });
const fullStamp = formatter({
  day: '2-digit',
  month: '2-digit',
  hour: '2-digit',
  minute: '2-digit',
  second: '2-digit',
});

// `seconds` matters at fine resolutions: over a 15-minute window every tick
// would otherwise read as the same HH:MM.
export function formatLocalTime(timestampMs, { seconds = false } = {}) {
  return (seconds ? hhmmss : hhmm).format(new Date(timestampMs));
}

export function formatLocalStamp(timestampMs) {
  return fullStamp.format(new Date(timestampMs));
}

// Local midnight, as ms since epoch. The race day starts at Czech midnight,
// not at 00:00 UTC (which is 02:00 local in summer) - the "dnes" range and
// the backend's default history window must agree on this.
export function startOfLocalDayMs(nowMs = Date.now()) {
  const parts = Object.fromEntries(
    formatter({ year: 'numeric', month: '2-digit', day: '2-digit', hour: '2-digit', minute: '2-digit', second: '2-digit' })
      .formatToParts(new Date(nowMs))
      .map((part) => [part.type, part.value]),
  );
  const localSecondsIntoDay = Number(parts.hour) * 3600 + Number(parts.minute) * 60 + Number(parts.second);
  const startMs = nowMs - localSecondsIntoDay * 1000;
  // Trim the sub-second remainder so the boundary is exactly on the second.
  return startMs - (startMs % 1000);
}
