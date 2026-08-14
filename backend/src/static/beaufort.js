// Beaufort scale, for the "so is that a lot?" question a raw m/s number does
// not answer for a troop leader briefing a crew. Descriptive only - the
// dashboard's disclaimer still stands, this is not a safety threshold and
// deliberately renders as plain text, not a red/green light.
//
// Boundaries are the standard scale's lower bounds in m/s.
const SCALE = [
  { force: 0, minMs: 0, name: 'bezvětří' },
  { force: 1, minMs: 0.3, name: 'vánek' },
  { force: 2, minMs: 1.6, name: 'slabý vítr' },
  { force: 3, minMs: 3.4, name: 'mírný vítr' },
  { force: 4, minMs: 5.5, name: 'dosti čerstvý vítr' },
  { force: 5, minMs: 8.0, name: 'čerstvý vítr' },
  { force: 6, minMs: 10.8, name: 'silný vítr' },
  { force: 7, minMs: 13.9, name: 'prudký vítr' },
  { force: 8, minMs: 17.2, name: 'bouřlivý vítr' },
  { force: 9, minMs: 20.8, name: 'vichřice' },
  { force: 10, minMs: 24.5, name: 'silná vichřice' },
  { force: 11, minMs: 28.5, name: 'mohutná vichřice' },
  { force: 12, minMs: 32.7, name: 'orkán' },
];

export function beaufort(windSpeedMs) {
  if (typeof windSpeedMs !== 'number' || !Number.isFinite(windSpeedMs) || windSpeedMs < 0) {
    return null;
  }
  // Highest step whose lower bound the reading has reached.
  let match = SCALE[0];
  for (const step of SCALE) {
    if (windSpeedMs >= step.minMs) {
      match = step;
    }
  }
  return match;
}

export function beaufortLabel(windSpeedMs) {
  const step = beaufort(windSpeedMs);
  return step === null ? '' : `${step.force} Bft · ${step.name}`;
}
