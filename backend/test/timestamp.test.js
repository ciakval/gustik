import { test } from 'node:test';
import assert from 'node:assert/strict';
import { normalizeCapturedAt } from '../src/ingest/timestamp.js';

test('normalizeCapturedAt passes a canonical UTC "Z" timestamp through unchanged', () => {
  assert.equal(normalizeCapturedAt('2026-08-01T09:00:00.000Z'), '2026-08-01T09:00:00.000Z');
});

test('normalizeCapturedAt accepts a non-UTC explicit offset and converts to UTC', () => {
  // +05:30 (IST) - 5.5 hours ahead of UTC
  assert.equal(normalizeCapturedAt('2026-08-01T14:30:00.000+05:30'), '2026-08-01T09:00:00.000Z');
});

test('normalizeCapturedAt accepts an offset with no milliseconds and pads to .000', () => {
  assert.equal(normalizeCapturedAt('2026-08-01T09:00:00Z'), '2026-08-01T09:00:00.000Z');
});

test('normalizeCapturedAt treats a naive timestamp in August as Europe/Prague summer time (CEST, +02:00)', () => {
  assert.equal(normalizeCapturedAt('2026-08-01T09:00:00'), '2026-08-01T07:00:00.000Z');
});

test('normalizeCapturedAt treats a naive timestamp in January as Europe/Prague winter time (CET, +01:00), not hardcoded CEST', () => {
  assert.equal(normalizeCapturedAt('2026-01-01T09:00:00'), '2026-01-01T08:00:00.000Z');
});

test('normalizeCapturedAt returns null for a non-ISO shape (space instead of "T", no zone)', () => {
  assert.equal(normalizeCapturedAt('2026-08-01 09:00:00'), null);
});

test('normalizeCapturedAt returns null for a non-string input', () => {
  assert.equal(normalizeCapturedAt(undefined), null);
  assert.equal(normalizeCapturedAt(12345), null);
});

test('normalizeCapturedAt returns null for garbage that merely starts with digits', () => {
  assert.equal(normalizeCapturedAt('2026-08-01T09:00:00.000Xyz'), null);
});
