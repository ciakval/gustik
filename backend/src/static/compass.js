// Wind direction presentation. Pure - no DOM, unit tested.
//
// Hard constraint (NFR-6 / AD-5): the vane hardware has exactly 8 physical
// steps. Nothing here may ever produce a direction finer than 45 degrees.
// Every angle below is octant * 45 by construction.

// Czech labels, per Mlok 2026-08-14 - the rest of the UI is Czech and the
// audience is Czech scout organizers. The trap this creates: Czech "S" is
// sever/north where English "S" is south. Mitigated by always rendering the
// full word alongside the abbreviation, never the abbreviation alone.
export const OCTANT_LABELS = ['S', 'SV', 'V', 'JV', 'J', 'JZ', 'Z', 'SZ'];

export const OCTANT_NAMES = [
  'sever',
  'severovýchod',
  'východ',
  'jihovýchod',
  'jih',
  'jihozápad',
  'západ',
  'severozápad',
];

export const DEGREES_PER_OCTANT = 45;

function isOctant(octant) {
  return Number.isInteger(octant) && octant >= 0 && octant < 8;
}

export function octantLabel(octant) {
  return isOctant(octant) ? OCTANT_LABELS[octant] : '–';
}

export function octantName(octant) {
  return isOctant(octant) ? OCTANT_NAMES[octant] : 'neznámý směr';
}

/**
 * Compass bearing the wind blows FROM, in degrees clockwise from north.
 * Always an exact multiple of 45 - never an interpolated value.
 */
export function octantToDegrees(octant) {
  return isOctant(octant) ? octant * DEGREES_PER_OCTANT : null;
}

/**
 * Rotation (deg, clockwise) for an arrow glyph whose artwork points "up",
 * so that it ends up pointing the way the wind TRAVELS.
 *
 * Meteorological convention reports where wind comes FROM; an arrow drawn
 * pointing that way reads backwards to most people ("the arrow points at the
 * wind's source"). Drawing it 180 degrees around makes it read as movement
 * across the map, which is what a sailor is actually thinking about. The UI
 * still labels the number as "vitr fouka od" so the two never contradict.
 */
export function octantArrowRotation(octant) {
  const degrees = octantToDegrees(octant);
  return degrees === null ? null : (degrees + 180) % 360;
}

/**
 * Circular (vector) mean of octants, snapped back to an octant.
 *
 * Same algorithm as the backend's serve/aggregate.js - duplicated on purpose,
 * the static/ bundle has no import path into src/serve/ and neither is worth
 * a build step over. Used when the client thins arrow glyphs on the chart:
 * dropping every Nth arrow would misreport a swinging wind, averaging the
 * ones being dropped does not.
 *
 * Direction is an angle, not a number: octants 7 (SZ) and 1 (SV) straddle
 * north and their arithmetic mean is 4 (J), the exact opposite of the truth.
 */
export function circularMeanOctant(octants) {
  const usable = octants.filter(isOctant);
  if (usable.length === 0) {
    return null;
  }
  let east = 0;
  let north = 0;
  for (const octant of usable) {
    const radians = (octant * DEGREES_PER_OCTANT * Math.PI) / 180;
    east += Math.sin(radians);
    north += Math.cos(radians);
  }
  // Exactly opposing samples sum to a zero vector, which has no direction at
  // all - atan2(0, 0) would silently answer "north". Report the first sample.
  if (Math.abs(east) < 1e-9 && Math.abs(north) < 1e-9) {
    return usable[0];
  }
  const degrees = ((Math.atan2(east, north) * 180) / Math.PI + 360) % 360;
  return Math.round(degrees / DEGREES_PER_OCTANT) % 8;
}
