import { msToKnots, isStale, formatAge, formatNumber } from './format.js';
import { octantLabel, octantName, octantToDegrees } from './compass.js';
import { beaufortLabel } from './beaufort.js';
import {
  initHistoryChart,
  setHistoryChartUnit,
  setHistoryChartWindow,
  setHistoryWindowResolver,
  handleLiveMessage,
  resyncHistoryChart,
} from './history-chart.js';
import { connectLiveSocket } from './live-socket.js';
import { mountTimeRange } from './timerange-ui.js';

const speedEl = document.getElementById('speed');
const speedSecondaryEl = document.getElementById('speed-secondary');
const beaufortEl = document.getElementById('beaufort');
const directionLabelEl = document.getElementById('direction-label');
const directionNameEl = document.getElementById('direction-name');
const arrowEl = document.getElementById('compass-arrow');
const ageEl = document.getElementById('age');
const unitMsBtn = document.getElementById('unit-ms');
const unitKtBtn = document.getElementById('unit-kt');

let latestReading = null;
let unit = 'ms';

function renderDirection(octant) {
  directionLabelEl.textContent = octantLabel(octant);
  directionNameEl.textContent = octantName(octant);
  const degrees = octantToDegrees(octant);
  if (degrees === null) {
    arrowEl.hidden = true;
    return;
  }
  // Always an exact multiple of 45 - the rose never implies a precision the
  // 8-position vane does not have.
  arrowEl.hidden = false;
  arrowEl.style.transform = `rotate(${degrees}deg)`;
}

function render() {
  if (!latestReading) {
    return;
  }
  const ms = latestReading.windSpeedMs;
  const primary = unit === 'ms' ? ms : msToKnots(ms);
  speedEl.textContent = `${formatNumber(primary)} ${unit === 'ms' ? 'm/s' : 'kt'}`;
  // The other unit stays visible rather than hidden behind the toggle -
  // organizers think in m/s, sailors in knots, and they read the same screen.
  speedSecondaryEl.textContent =
    unit === 'ms' ? `${formatNumber(msToKnots(ms))} uzlů` : `${formatNumber(ms)} m/s`;
  beaufortEl.textContent = beaufortLabel(ms);

  renderDirection(latestReading.windDirOctant);

  const stale = isStale(latestReading.capturedAt);
  ageEl.textContent = `naposledy před ${formatAge(latestReading.capturedAt)}`;
  ageEl.classList.toggle('stale', stale);
}

function setUnit(next) {
  unit = next;
  unitMsBtn.setAttribute('aria-pressed', String(unit === 'ms'));
  unitKtBtn.setAttribute('aria-pressed', String(unit === 'kt'));
  render();
  setHistoryChartUnit(unit);
}

unitMsBtn.addEventListener('click', () => setUnit('ms'));
unitKtBtn.addEventListener('click', () => setUnit('kt'));

async function fetchLatest() {
  const res = await fetch('/readings/latest');
  const { reading } = await res.json();
  if (reading) {
    latestReading = reading;
    render();
  }
}

// Age display keeps advancing even with no new data, so staleness is
// visible without needing a fresh message to trigger a re-render.
setInterval(render, 1000);

initHistoryChart({ emptyElement: document.getElementById('history-empty') });
const timeRange = mountTimeRange(document.getElementById('timerange'), (nextWindow) => {
  setHistoryChartWindow(nextWindow);
});
// Every range except "dnes" is relative to now, so a resync has to re-resolve
// the bounds rather than replay the ones captured at page load.
setHistoryWindowResolver(() => timeRange.refresh());

fetchLatest();

// One shared WS connection (Story 3.3) instead of dashboard.js and
// history-chart.js each opening their own - single reconnect lifecycle,
// so history-chart.js's resync-on-reconnect (AC2) actually happens instead
// of silently never reconnecting.
connectLiveSocket(`${location.protocol === 'https:' ? 'wss:' : 'ws:'}//${location.host}/readings/live`, {
  onOpen: () => {
    // WS is a best-effort optimization (AD-6) - on connect/reconnect always
    // re-sync against the REST source of truth instead of trusting WS alone.
    fetchLatest();
    resyncHistoryChart();
  },
  onMessage: (msg) => {
    if (msg.event) {
      handleLiveMessage(msg);
      return;
    }
    latestReading = msg;
    render();
  },
});

// A rolling window has to keep rolling even while data is arriving smoothly:
// without this the right-hand edge of the chart would freeze at page-load
// time and new readings would pile up outside the visible range.
setInterval(() => timeRange.refresh(), 30_000);
