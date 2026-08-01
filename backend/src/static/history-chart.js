import { buildSpeedPoints, buildDirectionPoints } from './history-chart-data.js';

const OCTANT_LABELS = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW'];

let chart = null;
let currentReadings = [];
let currentUnit = 'ms';

function speedAxisLabel(unit) {
  return unit === 'kt' ? 'Rychlost (uzly)' : 'Rychlost (m/s)';
}

function formatTimeTick(timestampMs) {
  return new Date(timestampMs).toISOString().slice(11, 16);
}

function render() {
  const speedPoints = buildSpeedPoints(currentReadings, currentUnit);
  const directionPoints = buildDirectionPoints(currentReadings);

  if (!chart) {
    const ctx = document.getElementById('history-chart');
    chart = new Chart(ctx, {
      type: 'line',
      data: {
        datasets: [
          {
            label: speedAxisLabel(currentUnit),
            data: speedPoints,
            yAxisID: 'speed',
            borderColor: '#4fb0ff',
            backgroundColor: '#4fb0ff',
            tension: 0.2,
            pointRadius: 1,
          },
          {
            label: 'Směr',
            data: directionPoints,
            yAxisID: 'direction',
            borderColor: '#8a97ab',
            backgroundColor: '#8a97ab',
            pointRadius: 1,
            showLine: false,
          },
        ],
      },
      options: {
        parsing: false,
        scales: {
          // plain linear axis over ms-since-epoch, not Chart.js's 'time'
          // scale - that needs a separate date-adapter library we don't
          // vendor (see history-chart-data.js)
          x: { type: 'linear', ticks: { callback: formatTimeTick } },
          speed: { type: 'linear', position: 'left', title: { display: true, text: speedAxisLabel(currentUnit) } },
          direction: {
            type: 'linear',
            position: 'right',
            min: -0.5,
            max: 7.5,
            grid: { drawOnChartArea: false },
            ticks: {
              stepSize: 1,
              callback: (value) => OCTANT_LABELS[value] ?? '',
            },
          },
        },
      },
    });
    return;
  }

  chart.data.datasets[0].data = speedPoints;
  chart.data.datasets[0].label = speedAxisLabel(currentUnit);
  chart.options.scales.speed.title.text = speedAxisLabel(currentUnit);
  chart.data.datasets[1].data = directionPoints;
  chart.update();
}

export function setHistoryChartReadings(readings) {
  currentReadings = readings;
  render();
}

export function setHistoryChartUnit(unit) {
  currentUnit = unit;
  render();
}

async function fetchHistory() {
  const res = await fetch('/readings/history');
  const { readings } = await res.json();
  setHistoryChartReadings(readings);
}

// Initial load on page open.
export function initHistoryChart() {
  fetchHistory();
}

// Story 3.3 AC1: history-changed carries no payload (AD-9) - just refetch
// and fully redraw, never attempt an incremental patch. Called from
// dashboard.js's shared live-socket onMessage handler.
export function handleLiveMessage(msg) {
  if (msg.event === 'history-changed') {
    fetchHistory();
  }
}

// Story 3.3 AC2: resync on every (re)connect, not just on a caught event -
// covers the case where history-changed fired while the socket was down
// (AD-6). Called from dashboard.js's shared live-socket onOpen handler.
export function resyncHistoryChart() {
  fetchHistory();
}
