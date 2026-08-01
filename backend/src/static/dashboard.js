import { msToKnots, octantToCompassLabel, isStale, formatAge } from './format.js';
import { initHistoryChart, setHistoryChartUnit } from './history-chart.js';

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

function connectLive() {
  const protocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
  const ws = new WebSocket(`${protocol}//${location.host}/readings/live`);

  ws.addEventListener('message', (event) => {
    latestReading = JSON.parse(event.data);
    render();
  });

  // WS is a best-effort optimization (AD-6) - on connect/reconnect always
  // re-sync against the REST source of truth instead of trusting WS alone.
  ws.addEventListener('open', fetchLatest);
  ws.addEventListener('close', () => setTimeout(connectLive, 2000));
}

// Age display keeps advancing even with no new data, so staleness is
// visible without needing a fresh message to trigger a re-render.
setInterval(render, 1000);

fetchLatest();
connectLive();
initHistoryChart();
