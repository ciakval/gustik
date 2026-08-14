import { test } from 'node:test';
import assert from 'node:assert/strict';
import {
  buildSpeedPoints,
  buildGustBand,
  buildDirectionArrows,
  buildRssiPoints,
  DIRECTION_STRIP_Y,
} from '../src/static/history-chart-data.js';

const READINGS = [
  { capturedAt: '2026-08-01T09:00:00.000Z', windSpeedMs: 2, windDirOctant: 0, rssiDbm: -50 },
  { capturedAt: '2026-08-01T09:05:00.000Z', windSpeedMs: 4, windDirOctant: 3, rssiDbm: -55 },
];

const BUCKETED = [
  {
    capturedAt: '2026-08-01T09:00:00.000Z',
    windSpeedMs: 3,
    windSpeedMinMs: 1,
    windSpeedMaxMs: 7,
    windDirOctant: 0,
    rssiDbm: -50,
    sampleCount: 20,
  },
];

test('buildSpeedPoints maps readings to {x,y} points in m/s by default, x as a numeric timestamp', () => {
  const points = buildSpeedPoints(READINGS, 'ms');
  assert.deepEqual(points, [
    { x: Date.parse('2026-08-01T09:00:00.000Z'), y: 2 },
    { x: Date.parse('2026-08-01T09:05:00.000Z'), y: 4 },
  ]);
});

test('buildSpeedPoints converts to knots client-side when unit is kt', () => {
  const points = buildSpeedPoints(READINGS, 'kt');
  assert.ok(Math.abs(points[0].y - 3.8877) < 0.001);
});

test('buildGustBand returns min and max series for bucketed data, converted with the unit', () => {
  const band = buildGustBand(BUCKETED, 'ms');
  assert.equal(band.max[0].y, 7);
  assert.equal(band.min[0].y, 1);

  const knots = buildGustBand(BUCKETED, 'kt');
  assert.ok(Math.abs(knots.max[0].y - 13.6069) < 0.001);
});

test('buildGustBand returns null for raw, unbucketed readings', () => {
  // min == max == the reading itself; a zero-width ribbon is noise.
  assert.equal(buildGustBand(READINGS, 'ms'), null);
  assert.equal(buildGustBand([], 'ms'), null);
});

test('buildDirectionArrows emits one rotated arrow per reading when under the cap', () => {
  const arrows = buildDirectionArrows(READINGS);
  assert.equal(arrows.length, 2);
  assert.deepEqual(arrows[0], {
    x: Date.parse('2026-08-01T09:00:00.000Z'),
    y: DIRECTION_STRIP_Y,
    octant: 0,
    rotation: 180,
  });
  assert.equal(arrows[1].octant, 3);
});

test('buildDirectionArrows thins to the cap by averaging groups, not by dropping samples', () => {
  const many = Array.from({ length: 100 }, (_, i) => ({
    capturedAt: new Date(Date.parse('2026-08-01T09:00:00.000Z') + i * 3000).toISOString(),
    windSpeedMs: 1,
    // Alternating around north - a "keep every Nth" thinning would report
    // whichever side the stride happened to land on.
    windDirOctant: i % 2 === 0 ? 7 : 1,
  }));
  const arrows = buildDirectionArrows(many, { maxArrows: 10 });
  assert.ok(arrows.length <= 10, `got ${arrows.length} arrows`);
  for (const arrow of arrows) {
    assert.equal(arrow.octant, 0, 'the averaged direction is north, not one of the extremes');
  }
});

test('buildDirectionArrows only ever produces rotations on the 45-degree grid (NFR-6)', () => {
  const arrows = buildDirectionArrows(
    Array.from({ length: 8 }, (_, octant) => ({
      capturedAt: new Date(Date.parse('2026-08-01T09:00:00.000Z') + octant * 60000).toISOString(),
      windSpeedMs: 1,
      windDirOctant: octant,
    })),
  );
  for (const arrow of arrows) {
    assert.equal(arrow.rotation % 45, 0, `rotation ${arrow.rotation}`);
  }
});

test('buildRssiPoints skips null rssi instead of plotting it as zero', () => {
  const points = buildRssiPoints([
    { capturedAt: '2026-08-01T09:00:00.000Z', rssiDbm: -60 },
    { capturedAt: '2026-08-01T09:00:03.000Z', rssiDbm: null },
  ]);
  assert.equal(points.length, 1);
  assert.equal(points[0].y, -60);
});

test('every builder handles an empty history without error', () => {
  assert.deepEqual(buildSpeedPoints([], 'ms'), []);
  assert.deepEqual(buildDirectionArrows([]), []);
  assert.deepEqual(buildRssiPoints([]), []);
});
