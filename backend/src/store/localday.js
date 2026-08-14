// The dashboard's "today" is the Czech race day, not the UTC day. Before
// this, GET /readings/history cut at 00:00 UTC, which in Europe/Prague is
// 02:00 local in summer - the graph reset two hours into the morning.
//
// No date library: Intl already knows every timezone rule, including DST.

export const DEFAULT_TIMEZONE = 'Europe/Prague';

// How far ahead of UTC the given zone is at the given instant, in ms.
// Works by asking Intl to render the instant in that zone, then reading the
// rendered wall-clock back as if it were UTC - the difference is the offset.
function zoneOffsetMs(date, timeZone) {
  const parts = Object.fromEntries(
    new Intl.DateTimeFormat('en-US', {
      timeZone,
      hourCycle: 'h23',
      year: 'numeric',
      month: '2-digit',
      day: '2-digit',
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit',
    })
      .formatToParts(date)
      .map((part) => [part.type, part.value]),
  );
  const asIfUtc = Date.UTC(
    Number(parts.year),
    Number(parts.month) - 1,
    Number(parts.day),
    Number(parts.hour),
    Number(parts.minute),
    Number(parts.second),
  );
  return asIfUtc - date.getTime();
}

/** The UTC instant at which the current local day started, as a canonical ISO string. */
export function startOfLocalDayIso(now = new Date(), timeZone = DEFAULT_TIMEZONE) {
  const offset = zoneOffsetMs(now, timeZone);
  const localWallClock = new Date(now.getTime() + offset);
  const localMidnightAsIfUtc = Date.UTC(
    localWallClock.getUTCFullYear(),
    localWallClock.getUTCMonth(),
    localWallClock.getUTCDate(),
  );
  // Re-read the offset AT the candidate instant rather than reusing `now`'s:
  // on a DST-transition day the two differ by an hour and the naive answer
  // lands on 01:00 or 23:00 local instead of midnight.
  const candidate = new Date(localMidnightAsIfUtc - offset);
  return new Date(localMidnightAsIfUtc - zoneOffsetMs(candidate, timeZone)).toISOString();
}
