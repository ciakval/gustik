import { msToKnots } from './format.js';

// x is a numeric timestamp (ms since epoch), not the ISO string or Chart.js's
// 'time' scale - that scale needs a separate date-adapter library we don't
// vendor. A plain 'linear' x-axis with numeric x values needs no adapter and
// keeps history-chart.js's Chart.js dependency to the single vendored file.
export function buildSpeedPoints(readings, unit) {
  return readings.map((r) => ({
    x: Date.parse(r.capturedAt),
    y: unit === 'kt' ? msToKnots(r.windSpeedMs) : r.windSpeedMs,
  }));
}

export function buildDirectionPoints(readings) {
  return readings.map((r) => ({ x: Date.parse(r.capturedAt), y: r.windDirOctant }));
}
