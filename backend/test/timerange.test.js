import { test } from 'node:test';
import assert from 'node:assert/strict';
import {
  RANGES,
  DEFAULT_RANGE_ID,
  rangeById,
  autoBucketSeconds,
  bucketOptionsFor,
  formatBucketSeconds,
  windowFor,
  loadSelection,
  saveSelection,
  BUCKET_LADDER_SECONDS,
} from '../src/static/timerange.js';
import { BUCKET_LADDER_SECONDS as BACKEND_LADDER } from '../src/serve/aggregate.js';

test('the client and server bucket ladders match', () => {
  // They are duplicated (no shared import path between static/ and serve/);
  // if they ever drift, an auto-chosen bucket and a server-clamped one stop
  // landing on the same round numbers.
  assert.deepEqual(BUCKET_LADDER_SECONDS, BACKEND_LADDER);
});

test('the default range is 1 hour', () => {
  assert.equal(DEFAULT_RANGE_ID, '1h');
  assert.equal(rangeById(DEFAULT_RANGE_ID).seconds, 3600);
});

test('an unknown range id falls back to the default rather than crashing', () => {
  assert.equal(rangeById('nonsense').id, DEFAULT_RANGE_ID);
  assert.equal(rangeById(undefined).id, DEFAULT_RANGE_ID);
});

test('every range except "dnes" has a fixed duration', () => {
  const openEnded = RANGES.filter((range) => range.seconds === null);
  assert.deepEqual(
    openEnded.map((range) => range.id),
    ['today'],
  );
});

test('autoBucketSeconds keeps the point count at or below the target', () => {
  for (const range of [900, 3600, 3 * 3600, 6 * 3600, 12 * 3600, 24 * 3600]) {
    const bucket = autoBucketSeconds(range, 180);
    assert.ok(range / bucket <= 180, `${range}s / ${bucket}s = ${range / bucket} points`);
    assert.ok(BUCKET_LADDER_SECONDS.includes(bucket), `${bucket} is a ladder value`);
  }
});

test('autoBucketSeconds never goes below the 3s firmware sample interval', () => {
  assert.equal(autoBucketSeconds(60, 180), 3);
  assert.equal(autoBucketSeconds(1, 180), 3);
});

test('autoBucketSeconds picks a sensible resolution for the 1h default', () => {
  // 3600s / 180 points = 20s ideal, snapped up to the ladder's 30s.
  assert.equal(autoBucketSeconds(3600, 180), 30);
});

test('bucketOptionsFor drops resolutions too coarse to draw a line with', () => {
  const options = bucketOptionsFor(900);
  assert.ok(options.includes(3));
  assert.ok(options.includes(60));
  // 900s / 3600s would be a quarter of one point.
  assert.ok(!options.includes(3600));
});

test('formatBucketSeconds renders human units', () => {
  assert.equal(formatBucketSeconds(30), '30 s');
  assert.equal(formatBucketSeconds(300), '5 min');
  assert.equal(formatBucketSeconds(3600), '1 h');
});

test('windowFor resolves a fixed range backwards from now', () => {
  const nowMs = Date.parse('2026-08-14T12:00:00.000Z');
  const window = windowFor({ rangeId: '1h', nowMs, startOfDayMs: 0 });
  assert.equal(window.from, '2026-08-14T11:00:00.000Z');
  assert.equal(window.to, '2026-08-14T12:00:00.000Z');
  assert.equal(window.rangeSeconds, 3600);
});

test('windowFor anchors "dnes" to the caller-supplied local midnight, not to a fixed duration', () => {
  const nowMs = Date.parse('2026-08-14T12:00:00.000Z');
  // Prague summer midnight is 22:00 UTC the previous day.
  const startOfDayMs = Date.parse('2026-08-13T22:00:00.000Z');
  const window = windowFor({ rangeId: 'today', nowMs, startOfDayMs });
  assert.equal(window.from, '2026-08-13T22:00:00.000Z');
  assert.equal(window.rangeSeconds, 14 * 3600);
});

test('windowFor honours an explicit bucket override and auto-derives otherwise', () => {
  const nowMs = Date.parse('2026-08-14T12:00:00.000Z');
  assert.equal(windowFor({ rangeId: '1h', nowMs, startOfDayMs: 0 }).bucketSeconds, 30);
  assert.equal(windowFor({ rangeId: '1h', bucketSeconds: 300, nowMs, startOfDayMs: 0 }).bucketSeconds, 300);
  assert.equal(windowFor({ rangeId: '1h', bucketSeconds: '600', nowMs, startOfDayMs: 0 }).bucketSeconds, 600);
});

function fakeStorage() {
  const map = new Map();
  return {
    getItem: (k) => map.get(k) ?? null,
    setItem: (k, v) => map.set(k, v),
  };
}

test('the selection round-trips through storage', () => {
  const storage = fakeStorage();
  saveSelection({ rangeId: '6h', bucketSeconds: 300 }, storage);
  assert.deepEqual(loadSelection(storage), { rangeId: '6h', bucketSeconds: '300' });
});

test('loading with nothing stored yields the defaults', () => {
  assert.deepEqual(loadSelection(fakeStorage()), { rangeId: DEFAULT_RANGE_ID, bucketSeconds: 'auto' });
});

test('a storage that throws (private mode) falls back to defaults instead of breaking the page', () => {
  const hostile = {
    getItem() {
      throw new Error('SecurityError');
    },
    setItem() {
      throw new Error('SecurityError');
    },
  };
  assert.deepEqual(loadSelection(hostile), { rangeId: DEFAULT_RANGE_ID, bucketSeconds: 'auto' });
  assert.doesNotThrow(() => saveSelection({ rangeId: '1h', bucketSeconds: 'auto' }, hostile));
});
