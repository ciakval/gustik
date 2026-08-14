import { test } from 'node:test';
import assert from 'node:assert/strict';
import { bucketReadings, circularMeanOctant, clampBucketSeconds } from '../src/serve/aggregate.js';

function reading(iso, { speed = 0, octant = 0, rssi = -60 } = {}) {
  return { capturedAt: iso, windSpeedMs: speed, windDirOctant: octant, rssiDbm: rssi };
}

test('circularMeanOctant averages as a direction, not as a number', () => {
  // The whole point: 7 (SZ) and 1 (SV) straddle north. A plain arithmetic
  // mean gives 4 (J) - the exact opposite of the truth.
  assert.equal(circularMeanOctant([7, 1]), 0);
  assert.equal(circularMeanOctant([0, 0, 0]), 0);
  assert.equal(circularMeanOctant([1, 2]), 2); // ties snap up, deterministically
  assert.equal(circularMeanOctant([2, 2, 2, 4]), 2);
});

test('circularMeanOctant returns null for no samples', () => {
  assert.equal(circularMeanOctant([]), null);
});

test('circularMeanOctant falls back to the first sample when directions cancel out exactly', () => {
  // Opposite directions sum to a zero vector - atan2(0,0) is 0, which would
  // silently claim "north". Report the first sample instead of inventing one.
  assert.equal(circularMeanOctant([2, 6]), 2);
  assert.equal(circularMeanOctant([5, 1]), 5);
});

test('bucketReadings groups into fixed-width buckets stamped at the bucket start', () => {
  const readings = [
    reading('2026-08-14T10:00:01.000Z', { speed: 2 }),
    reading('2026-08-14T10:00:29.000Z', { speed: 4 }),
    reading('2026-08-14T10:00:31.000Z', { speed: 6 }),
  ];
  const buckets = bucketReadings(readings, 30);
  assert.equal(buckets.length, 2);
  assert.equal(buckets[0].capturedAt, '2026-08-14T10:00:00.000Z');
  assert.equal(buckets[1].capturedAt, '2026-08-14T10:00:30.000Z');
});

test('bucketReadings reports mean, min and max speed per bucket (gust band)', () => {
  const readings = [
    reading('2026-08-14T10:00:00.000Z', { speed: 2 }),
    reading('2026-08-14T10:00:10.000Z', { speed: 8 }),
    reading('2026-08-14T10:00:20.000Z', { speed: 5 }),
  ];
  const [bucket] = bucketReadings(readings, 60);
  assert.equal(bucket.windSpeedMs, 5);
  assert.equal(bucket.windSpeedMinMs, 2);
  assert.equal(bucket.windSpeedMaxMs, 8);
  assert.equal(bucket.sampleCount, 3);
});

test('bucketReadings omits empty buckets instead of zero-filling them', () => {
  // A gap must look like a gap on the chart, never like a calm.
  const readings = [
    reading('2026-08-14T10:00:00.000Z', { speed: 3 }),
    reading('2026-08-14T10:05:00.000Z', { speed: 3 }),
  ];
  const buckets = bucketReadings(readings, 60);
  assert.equal(buckets.length, 2);
  assert.deepEqual(
    buckets.map((b) => b.capturedAt),
    ['2026-08-14T10:00:00.000Z', '2026-08-14T10:05:00.000Z'],
  );
});

test('bucketReadings averages rssi, ignoring null samples', () => {
  const readings = [
    reading('2026-08-14T10:00:00.000Z', { rssi: -70 }),
    reading('2026-08-14T10:00:10.000Z', { rssi: null }),
    reading('2026-08-14T10:00:20.000Z', { rssi: -80 }),
  ];
  const [bucket] = bucketReadings(readings, 60);
  assert.equal(bucket.rssiDbm, -75);
});

test('bucketReadings reports null rssi when every sample in the bucket is null', () => {
  const readings = [
    reading('2026-08-14T10:00:00.000Z', { rssi: null }),
    reading('2026-08-14T10:00:10.000Z', { rssi: null }),
  ];
  const [bucket] = bucketReadings(readings, 60);
  assert.equal(bucket.rssiDbm, null);
});

test('bucketReadings uses the circular mean for direction', () => {
  const readings = [
    reading('2026-08-14T10:00:00.000Z', { octant: 7 }),
    reading('2026-08-14T10:00:10.000Z', { octant: 1 }),
  ];
  const [bucket] = bucketReadings(readings, 60);
  assert.equal(bucket.windDirOctant, 0);
});

test('bucketReadings returns the input untouched when no bucketing is requested', () => {
  const readings = [reading('2026-08-14T10:00:00.000Z', { speed: 3 })];
  assert.deepEqual(bucketReadings(readings, 0), readings);
  assert.deepEqual(bucketReadings(readings, null), readings);
});

test('bucketReadings handles an empty reading list', () => {
  assert.deepEqual(bucketReadings([], 60), []);
});

test('clampBucketSeconds raises the bucket so a range cannot exceed the point cap', () => {
  const oneDay = 24 * 60 * 60;
  // 3s buckets over a day would be 28800 points - well past the cap.
  const clamped = clampBucketSeconds(3, oneDay, 2000);
  assert.ok(clamped >= oneDay / 2000, `expected >= ${oneDay / 2000}, got ${clamped}`);
  assert.ok(oneDay / clamped <= 2000);
});

test('clampBucketSeconds leaves an already-reasonable bucket alone', () => {
  assert.equal(clampBucketSeconds(60, 3600, 2000), 60);
});
