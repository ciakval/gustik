// DOM half of the time-range control (the model is timerange.js). Mounted on
// both the dashboard and the status page - Mlok asked for the same control in
// both places, so it is one component rather than two similar ones.

import {
  RANGES,
  rangeById,
  bucketOptionsFor,
  formatBucketSeconds,
  windowFor,
  loadSelection,
  saveSelection,
} from './timerange.js';
import { startOfLocalDayMs } from './timezone.js';

/**
 * Renders the chip row + resolution select into `container` and calls
 * `onChange(window)` whenever the selection changes, where `window` is
 * {from, to, rangeSeconds, bucketSeconds}.
 *
 * Returns { currentWindow(), refresh() } - refresh() re-resolves the window
 * against the current clock, which is what a rolling "last 1 h" view needs
 * every time new data arrives.
 */
export function mountTimeRange(container, onChange) {
  let selection = loadSelection();

  const chips = document.createElement('div');
  chips.className = 'chips';
  chips.setAttribute('role', 'group');
  chips.setAttribute('aria-label', 'Časový rozsah');

  const chipButtons = RANGES.map((range) => {
    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'chip';
    button.textContent = range.label;
    button.dataset.rangeId = range.id;
    button.addEventListener('click', () => {
      // Changing the window changes what "auto" resolves to, so an explicit
      // override is dropped rather than carried across ranges where it may
      // make no sense (5 min buckets over a 15 min window = 3 points).
      selection = { rangeId: range.id, bucketSeconds: 'auto' };
      saveSelection(selection);
      syncControls();
      emit();
    });
    chips.appendChild(button);
    return button;
  });

  const resolution = document.createElement('label');
  resolution.className = 'resolution';
  const resolutionText = document.createElement('span');
  resolutionText.textContent = 'rozlišení';
  const select = document.createElement('select');
  select.setAttribute('aria-label', 'Rozlišení grafu');
  select.addEventListener('change', () => {
    selection = { ...selection, bucketSeconds: select.value };
    saveSelection(selection);
    syncControls();
    emit();
  });
  resolution.append(resolutionText, select);

  container.classList.add('timerange');
  container.append(chips, resolution);

  function resolveWindow() {
    return windowFor({
      rangeId: selection.rangeId,
      bucketSeconds: selection.bucketSeconds,
      nowMs: Date.now(),
      startOfDayMs: startOfLocalDayMs(),
    });
  }

  function syncControls() {
    const active = rangeById(selection.rangeId);
    for (const button of chipButtons) {
      button.setAttribute('aria-pressed', String(button.dataset.rangeId === active.id));
    }

    const window = resolveWindow();
    const options = bucketOptionsFor(window.rangeSeconds);
    // The auto option spells out what auto currently means, so a user can see
    // the effective resolution without having to override it to find out.
    select.innerHTML = '';
    const auto = new Option(`auto (${formatBucketSeconds(window.bucketSeconds)})`, 'auto');
    select.add(auto);
    for (const seconds of options) {
      select.add(new Option(formatBucketSeconds(seconds), String(seconds)));
    }
    // An override that no longer fits the new range falls back to auto.
    const wanted = String(selection.bucketSeconds);
    select.value = [...select.options].some((option) => option.value === wanted) ? wanted : 'auto';
    if (select.value === 'auto' && wanted !== 'auto') {
      selection = { ...selection, bucketSeconds: 'auto' };
      saveSelection(selection);
    }
  }

  let current = null;
  function emit() {
    current = resolveWindow();
    onChange(current);
  }

  syncControls();
  emit();

  return {
    currentWindow: () => current,
    refresh() {
      syncControls();
      emit();
    },
  };
}
