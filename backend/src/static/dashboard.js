import { msToKnots, octantToCompassLabel, isStale, formatAge } from './format.js';
import { initHistoryChart, setHistoryChartUnit, handleLiveMessage, resyncHistoryChart } from './history-chart.js';
import { connectLiveSocket } from './live-socket.js';

const speedEl = document.getElementById('speed');
const directionEl = document.getElementById('direction');
const ageEl = document.getElementById('age');
const unitMsBtn = document.getElementById('unit-ms');
const unitKtBtn = document.getElementById('unit-kt');

let latestReading = null;
let unit = 'ms';

function render() {
  if (!latestReading) {
    return;
  }
  const speed = unit === 'ms' ? latestReading.windSpeedMs : msToKnots(latestReading.windSpeedMs);
  speedEl.textContent = `${speed.toFixed(1)} ${unit === 'ms' ? 'm/s' : 'kt'}`;
  directionEl.textContent = octantToCompassLabel(latestReading.windDirOctant);

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

fetchLatest();
initHistoryChart();

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
