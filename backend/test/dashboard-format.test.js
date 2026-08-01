import { test } from 'node:test';
import assert from 'node:assert/strict';
import { msToKnots, octantToCompassLabel, isStale, formatAge } from '../src/static/format.js';

test('msToKnots converts m/s to knots', () => {
  assert.ok(Math.abs(msToKnots(1) - 1.943844) < 0.0001);
  assert.equal(msToKnots(0), 0);
});

test('octantToCompassLabel maps 0-7 to the 8 cardinal/intercardinal directions', () => {
  assert.equal(octantToCompassLabel(0), 'N');
  assert.equal(octantToCompassLabel(1), 'NE');
  assert.equal(octantToCompassLabel(2), 'E');
  assert.equal(octantToCompassLabel(3), 'SE');
  assert.equal(octantToCompassLabel(4), 'S');
  assert.equal(octantToCompassLabel(5), 'SW');
  assert.equal(octantToCompassLabel(6), 'W');
  assert.equal(octantToCompassLabel(7), 'NW');
});

test('isStale is false when the reading is under the 2-minute threshold (NFR-3)', () => {
  const capturedAt = '2026-08-01T09:00:00.000Z';
  const now = '2026-08-01T09:01:59.000Z'; // 119s later
  assert.equal(isStale(capturedAt, now), false);
});

test('isStale is true once a reading is 2 minutes old or older (NFR-3)', () => {
  const capturedAt = '2026-08-01T09:00:00.000Z';
  const now = '2026-08-01T09:02:00.000Z'; // exactly 120s later
  assert.equal(isStale(capturedAt, now), true);
});

test('formatAge renders seconds for recent readings', () => {
  const capturedAt = '2026-08-01T09:00:00.000Z';
  const now = '2026-08-01T09:00:45.000Z';
  assert.equal(formatAge(capturedAt, now), '45s');
});

test('formatAge renders minutes for older readings', () => {
  const capturedAt = '2026-08-01T09:00:00.000Z';
  const now = '2026-08-01T09:05:30.000Z';
  assert.equal(formatAge(capturedAt, now), '5min');
});
