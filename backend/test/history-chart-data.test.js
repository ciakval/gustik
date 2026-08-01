import { test } from 'node:test';
import assert from 'node:assert/strict';
import { buildSpeedPoints, buildDirectionPoints } from '../src/static/history-chart-data.js';

const READINGS = [
  { capturedAt: '2026-08-01T09:00:00.000Z', windSpeedMs: 2, windDirOctant: 0, rssiDbm: -50 },
  { capturedAt: '2026-08-01T09:05:00.000Z', windSpeedMs: 4, windDirOctant: 3, rssiDbm: -55 },
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

test('buildDirectionPoints maps readings to {x,y} points using the raw octant (0-7), never degrees', () => {
  const points = buildDirectionPoints(READINGS);
  assert.deepEqual(points, [
    { x: Date.parse('2026-08-01T09:00:00.000Z'), y: 0 },
    { x: Date.parse('2026-08-01T09:05:00.000Z'), y: 3 },
  ]);
});

test('buildSpeedPoints and buildDirectionPoints handle an empty history without error', () => {
  assert.deepEqual(buildSpeedPoints([], 'ms'), []);
  assert.deepEqual(buildDirectionPoints([]), []);
});
