import { msToKnots } from './format.js';
import { circularMeanOctant, octantArrowRotation } from './compass.js';

// x is a numeric timestamp (ms since epoch), not the ISO string or Chart.js's
// 'time' scale - that scale needs a separate date-adapter library we don't
// vendor. A plain 'linear' x-axis with numeric x values needs no adapter and
// keeps history-chart.js's Chart.js dependency to the single vendored file.

// Where the direction arrows sit on their own hidden 0..1 axis: near the top
// of the plot area, clear of the speed line.
export const DIRECTION_STRIP_Y = 0.92;

// Enough arrows to read a trend, few enough that they never overlap on a
// phone. Beyond this the readings are averaged into each arrow, not dropped.
export const MAX_DIRECTION_ARROWS = 24;

function convert(value, unit) {
  return unit === 'kt' ? msToKnots(value) : value;
}

export function buildSpeedPoints(readings, unit) {
  return readings.map((r) => ({
    x: Date.parse(r.capturedAt),
    y: convert(r.windSpeedMs, unit),
  }));
}

/**
 * Min and max series for the gust band. Returns null for raw (unbucketed)
 * data, where min == max == the reading itself and a band would be a
 * meaningless zero-width ribbon behind the line.
 */
export function buildGustBand(readings, unit) {
  if (readings.length === 0 || readings[0].windSpeedMaxMs === undefined) {
    return null;
  }
  return {
    max: readings.map((r) => ({ x: Date.parse(r.capturedAt), y: convert(r.windSpeedMaxMs, unit) })),
    min: readings.map((r) => ({ x: Date.parse(r.capturedAt), y: convert(r.windSpeedMinMs, unit) })),
  };
}

/**
 * Arrow glyphs for the direction strip, thinned to at most `maxArrows`.
 *
 * Thinning averages each group with the circular mean rather than keeping
 * every Nth sample: a wind swinging back and forth would otherwise be
 * reported by whichever sample happened to land on the sampling stride.
 *
 * `rotation` is a whole number of 45-degree steps by construction (NFR-6):
 * Chart.js rotates the point glyph by it, so a direction crossing north is
 * a 45-degree turn of the arrow, never a jump across an axis - which is
 * exactly what the previous linear 0..7 direction axis got wrong.
 */
export function buildDirectionArrows(readings, { maxArrows = MAX_DIRECTION_ARROWS } = {}) {
  if (readings.length === 0) {
    return [];
  }
  const stride = Math.ceil(readings.length / maxArrows);
  const arrows = [];
  for (let start = 0; start < readings.length; start += stride) {
    const group = readings.slice(start, start + stride);
    const octant = circularMeanOctant(group.map((r) => r.windDirOctant));
    if (octant === null) {
      continue;
    }
    // Stamp the arrow at the middle of the group it summarizes, not at its
    // leading edge, so it lines up with the stretch of line it describes.
    const middle = group[Math.floor(group.length / 2)];
    arrows.push({
      x: Date.parse(middle.capturedAt),
      y: DIRECTION_STRIP_Y,
      octant,
      rotation: octantArrowRotation(octant),
    });
  }
  return arrows;
}

export function buildRssiPoints(readings) {
  return readings
    .filter((r) => r.rssiDbm !== null && r.rssiDbm !== undefined)
    .map((r) => ({ x: Date.parse(r.capturedAt), y: r.rssiDbm }));
}
