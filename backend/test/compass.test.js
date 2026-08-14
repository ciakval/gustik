import { test } from 'node:test';
import assert from 'node:assert/strict';
import {
  octantLabel,
  octantName,
  octantToDegrees,
  octantArrowRotation,
  circularMeanOctant,
  OCTANT_LABELS,
} from '../src/static/compass.js';

test('octant labels are the Czech compass abbreviations, S = sever at index 0', () => {
  assert.deepEqual(OCTANT_LABELS, ['S', 'SV', 'V', 'JV', 'J', 'JZ', 'Z', 'SZ']);
  assert.equal(octantLabel(0), 'S');
  assert.equal(octantName(0), 'sever');
  assert.equal(octantLabel(4), 'J');
  assert.equal(octantName(4), 'jih');
  assert.equal(octantName(7), 'severozápad');
});

test('an out-of-range or missing octant renders as a placeholder, never as north', () => {
  for (const bad of [undefined, null, -1, 8, 2.5, 'S']) {
    assert.equal(octantLabel(bad), '–', `label for ${bad}`);
    assert.equal(octantName(bad), 'neznámý směr', `name for ${bad}`);
    assert.equal(octantToDegrees(bad), null, `degrees for ${bad}`);
    assert.equal(octantArrowRotation(bad), null, `rotation for ${bad}`);
  }
});

test('octantToDegrees only ever produces exact multiples of 45 (NFR-6/AD-5)', () => {
  for (let octant = 0; octant < 8; octant += 1) {
    const degrees = octantToDegrees(octant);
    assert.equal(degrees % 45, 0);
    assert.equal(degrees, octant * 45);
  }
});

test('the arrow glyph points the way the wind travels, i.e. 180 degrees off the reported bearing', () => {
  // Wind FROM the north (octant 0) travels southward - an arrow whose art
  // points up must be rotated half a turn.
  assert.equal(octantArrowRotation(0), 180);
  assert.equal(octantArrowRotation(4), 0);
  assert.equal(octantArrowRotation(7), 135);
});

test('circularMeanOctant averages directions as angles, not as numbers', () => {
  // 7 (SZ) and 1 (SV) straddle north; the arithmetic mean 4 (J) is the exact
  // opposite of the truth.
  assert.equal(circularMeanOctant([7, 1]), 0);
  assert.equal(circularMeanOctant([3, 3, 3]), 3);
  assert.equal(circularMeanOctant([2, 2, 2, 4]), 2);
});

test('circularMeanOctant returns null when there is nothing usable to average', () => {
  assert.equal(circularMeanOctant([]), null);
  assert.equal(circularMeanOctant([null, undefined, 9]), null);
});

test('circularMeanOctant ignores unusable entries mixed in with real ones', () => {
  assert.equal(circularMeanOctant([null, 3, 3]), 3);
});

test('circularMeanOctant falls back to the first sample when directions cancel exactly', () => {
  assert.equal(circularMeanOctant([2, 6]), 2);
  assert.equal(circularMeanOctant([6, 2]), 6);
});
