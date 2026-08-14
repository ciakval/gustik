import { buildSpeedPoints, buildGustBand, buildDirectionArrows } from './history-chart-data.js';
import { formatLocalTime, formatLocalStamp } from './timezone.js';
import { octantLabel, octantName } from './compass.js';
import { formatNumber } from './format.js';

const ACCENT = '#4fb0ff';
const BAND = 'rgba(79, 176, 255, 0.16)';
const MUTED = '#8a97ab';
const GRID = 'rgba(34, 48, 74, 0.6)';

let chart = null;
let readings = [];
let unit = 'ms';
let currentWindow = null;
let fetchWindow = null;
let emptyEl = null;

function speedAxisLabel() {
  return unit === 'kt' ? 'Rychlost (uzly)' : 'Rychlost (m/s)';
}

function unitSuffix() {
  return unit === 'kt' ? 'uzlů' : 'm/s';
}

// Seconds on the tick labels only when the window is short enough that
// every tick would otherwise read as the same HH:MM.
function showSeconds() {
  return (currentWindow?.rangeSeconds ?? 3600) <= 15 * 60;
}

function directionTooltip(context) {
  const arrow = context.raw;
  return `Směr: ${octantLabel(arrow.octant)} (${octantName(arrow.octant)})`;
}

function speedTooltip(context) {
  const point = context.raw;
  const bucket = readings[context.dataIndex];
  const value = `${formatNumber(point.y)} ${unitSuffix()}`;
  if (!bucket || bucket.windSpeedMaxMs === undefined) {
    return `Rychlost: ${value}`;
  }
  const band = buildGustBand([bucket], unit);
  return `Průměr ${value} · náraz ${formatNumber(band.max[0].y)} ${unitSuffix()} (${bucket.sampleCount}×)`;
}

function datasets() {
  const band = buildGustBand(readings, unit);
  const list = [];

  if (band) {
    // Drawn first, behind the average line. The max series fills DOWN to the
    // min series (fill: '+1' = the dataset after this one), which is what
    // renders the shaded gust ribbon - min itself draws nothing.
    list.push({
      label: 'Nárazy (min–max)',
      data: band.max,
      yAxisID: 'speed',
      borderWidth: 0,
      pointRadius: 0,
      fill: '+1',
      backgroundColor: BAND,
      order: 3,
    });
    list.push({
      label: '_gust-min',
      data: band.min,
      yAxisID: 'speed',
      borderWidth: 0,
      pointRadius: 0,
      fill: false,
      order: 3,
    });
  }

  list.push({
    label: band ? 'Průměrná rychlost' : 'Rychlost',
    data: buildSpeedPoints(readings, unit),
    yAxisID: 'speed',
    borderColor: ACCENT,
    backgroundColor: ACCENT,
    borderWidth: 2,
    tension: 0.25,
    pointRadius: 0,
    pointHitRadius: 12,
    order: 2,
  });

  // Arrow density follows the plot width, not a fixed count: 24 arrows read
  // well on a laptop and overlap into a smear on a 390px phone.
  const plotWidth = chart?.chartArea?.width ?? document.getElementById('history-chart')?.clientWidth ?? 700;
  const arrows = buildDirectionArrows(readings, { maxArrows: Math.max(5, Math.floor(plotWidth / 34)) });
  list.push({
    label: 'Směr větru',
    data: arrows,
    yAxisID: 'direction',
    showLine: false,
    borderColor: MUTED,
    backgroundColor: MUTED,
    pointStyle: 'triangle',
    pointRadius: 7,
    // Per-point rotation is the whole trick: direction is a circular
    // quantity, so it is drawn as an angle, not plotted on a numeric axis
    // where a swing across north jumps from one edge to the other.
    rotation: arrows.map((arrow) => arrow.rotation),
    order: 1,
  });

  return list;
}

function options() {
  return {
    parsing: false,
    maintainAspectRatio: false,
    animation: false,
    interaction: { mode: 'nearest', axis: 'x', intersect: false },
    scales: {
      // plain linear axis over ms-since-epoch, not Chart.js's 'time'
      // scale - that needs a separate date-adapter library we don't
      // vendor (see history-chart-data.js)
      x: {
        type: 'linear',
        min: currentWindow ? Date.parse(currentWindow.from) : undefined,
        max: currentWindow ? Date.parse(currentWindow.to) : undefined,
        ticks: {
          color: MUTED,
          maxRotation: 0,
          autoSkipPadding: 24,
          callback: (value) => formatLocalTime(value, { seconds: showSeconds() }),
        },
        grid: { color: GRID },
      },
      speed: {
        type: 'linear',
        position: 'left',
        beginAtZero: true,
        title: { display: true, text: speedAxisLabel(), color: MUTED },
        ticks: { color: MUTED },
        grid: { color: GRID },
      },
      // Hidden carrier axis for the arrow strip - the arrows encode
      // direction in their rotation, so this axis has no meaning to show.
      // Headroom above DIRECTION_STRIP_Y keeps the glyphs off the top edge.
      direction: {
        type: 'linear',
        position: 'right',
        min: 0,
        max: 1,
        display: false,
      },
    },
    plugins: {
      legend: {
        labels: {
          color: MUTED,
          usePointStyle: true,
          // The min series exists only as a fill target; showing it in the
          // legend would offer a toggle that visually breaks the band.
          filter: (item) => item.text !== '_gust-min',
        },
      },
      tooltip: {
        callbacks: {
          title: (items) => formatLocalStamp(items[0].parsed.x),
          label: (context) =>
            context.dataset.yAxisID === 'direction' ? directionTooltip(context) : speedTooltip(context),
        },
        filter: (item) => item.dataset.label !== '_gust-min',
      },
    },
  };
}

function render() {
  if (emptyEl) {
    emptyEl.hidden = readings.length > 0;
  }
  if (!chart) {
    chart = new Chart(document.getElementById('history-chart'), {
      type: 'line',
      data: { datasets: datasets() },
      options: options(),
    });
    return;
  }
  chart.data.datasets = datasets();
  chart.options = options();
  chart.update();
}

export function setHistoryChartReadings(next) {
  readings = next;
  render();
}

export function setHistoryChartUnit(next) {
  unit = next;
  render();
}

async function fetchHistory() {
  if (!currentWindow) {
    return;
  }
  const query = new URLSearchParams({
    from: currentWindow.from,
    to: currentWindow.to,
    bucket: String(currentWindow.bucketSeconds),
  });
  const res = await fetch(`/readings/history?${query}`);
  const body = await res.json();
  // The server may have coarsened the bucket to stay under its point cap -
  // report what was actually used, never what was asked for.
  if (body.bucketSeconds && body.bucketSeconds !== currentWindow.bucketSeconds) {
    currentWindow = { ...currentWindow, bucketSeconds: body.bucketSeconds };
  }
  setHistoryChartReadings(body.readings ?? []);
  return body;
}

/**
 * Point the chart at a new window ({from, to, rangeSeconds, bucketSeconds})
 * and reload. Called by the time-range control and on every live update, so
 * a rolling "last 1 h" view actually rolls.
 */
export function setHistoryChartWindow(next) {
  currentWindow = next;
  return fetchHistory();
}

export function initHistoryChart({ emptyElement } = {}) {
  emptyEl = emptyElement ?? null;
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
  if (fetchWindow) {
    // The window is time-relative, so a resync after a reconnect must
    // re-resolve "last 1 h" against the current clock, not replay the stale
    // bounds captured when the page loaded.
    fetchWindow();
    return;
  }
  fetchHistory();
}

/** Lets the page hand back its "re-resolve the window against now" function. */
export function setHistoryWindowResolver(resolver) {
  fetchWindow = resolver;
}
