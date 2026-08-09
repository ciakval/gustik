// Single named seam for the dashboard's display timezone. A future
// per-user-preference override (e.g. read from localStorage) only needs to
// change what this resolves to - see CLAUDE.md "Choosing a workflow" /
// the timestamp-timezone-support design doc.
export const DISPLAY_TIMEZONE = 'Europe/Prague';

const timeFormatter = new Intl.DateTimeFormat('en-GB', {
  timeZone: DISPLAY_TIMEZONE,
  hour: '2-digit',
  minute: '2-digit',
  hourCycle: 'h23',
});

export function formatLocalTime(timestampMs) {
  return timeFormatter.format(new Date(timestampMs));
}
