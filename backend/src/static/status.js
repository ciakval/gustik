// Diagnostics page. Deliberately shows raw values next to their
// interpretation - every firmware bug so far (bug-028..031) was diagnosed
// from exactly these fields, read by hand off curl output.

import { formatAge, isStale, msToKnots, formatNumber } from './format.js';
import { octantLabel, octantName } from './compass.js';
import { formatLocalStamp, formatLocalTime } from './timezone.js';
import { buildRssiPoints } from './history-chart-data.js';
import { mountTimeRange } from './timerange-ui.js';
import { connectLiveSocket } from './live-socket.js';

const MUTED = '#8a97ab';
const ACCENT = '#4fb0ff';
const GRID = 'rgba(34, 48, 74, 0.6)';
// Rough practical thresholds for 2.4GHz Wi-Fi, drawn as reference lines so a
// number that means nothing to a non-engineer still reads as fine / not fine.
const RSSI_GOOD = -67;
const RSSI_MARGINAL = -80;

const latestEl = document.getElementById('latest');
const totalsEl = document.getElementById('totals');
const logEl = document.getElementById('log');
const logEmptyEl = document.getElementById('log-empty');
const recentEl = document.getElementById('recent');
const recentEmptyEl = document.getElementById('recent-empty');
const rssiEmptyEl = document.getElementById('rssi-empty');

let rssiChart = null;
let currentWindow = null;
let lastStatus = null;

function el(tag, { text, className } = {}) {
  const node = document.createElement(tag);
  if (text !== undefined) node.textContent = text;
  if (className) node.className = className;
  return node;
}

function badge(text, tone) {
  return el('span', { text, className: `badge ${tone}` });
}

function row(list, term, value) {
  list.append(el('dt', { text: term }));
  const dd = el('dd');
  dd.append(value instanceof Node ? value : el('span', { text: String(value) }));
  list.append(dd);
}

function renderLatest(latest) {
  latestEl.innerHTML = '';
  if (!latest) {
    row(latestEl, 'stav', badge('žádná data', 'bad'));
    return;
  }
  const stale = isStale(latest.capturedAt);
  row(latestEl, 'stav', stale ? badge('data zastarala', 'bad') : badge('živá data', 'ok'));
  row(latestEl, 'stáří', `${formatAge(latest.capturedAt)}`);
  row(latestEl, 'změřeno', formatLocalStamp(Date.parse(latest.capturedAt)));
  row(latestEl, 'přijato serverem', formatLocalStamp(Date.parse(latest.receivedAt)));
  // The lag between the two is what a buffered/backfilled batch looks like.
  row(
    latestEl,
    'zpoždění přenosu',
    `${Math.round((Date.parse(latest.receivedAt) - Date.parse(latest.capturedAt)) / 1000)} s`,
  );
  row(latestEl, 'rychlost', `${formatNumber(latest.windSpeedMs, 2)} m/s · ${formatNumber(msToKnots(latest.windSpeedMs))} uzlů`);
  row(
    latestEl,
    'směr',
    `${octantLabel(latest.windDirOctant)} — ${octantName(latest.windDirOctant)} (oktant ${latest.windDirOctant})`,
  );
  row(latestEl, 'RSSI', latest.rssiDbm === null ? '—' : `${latest.rssiDbm} dBm`);
  row(
    latestEl,
    'hodiny (NTP)',
    latest.clockSynced ? badge('synchronizované', 'ok') : badge('nesynchronizované', 'warn'),
  );
  row(latestEl, 'backfill', latest.backfilled ? badge('ano', 'warn') : badge('ne', 'ok'));
  row(latestEl, 'clientId', latest.clientId);
}

function renderTotals(status) {
  totalsEl.innerHTML = '';
  row(totalsEl, 'čas serveru', formatLocalStamp(Date.parse(status.serverTimeIso)));
  row(totalsEl, 'záznamů celkem', status.totals.readings);
  row(totalsEl, 'z toho backfill', status.totals.backfilled);
  row(totalsEl, 'z toho bez NTP', status.totals.clockUnsynced);
  row(totalsEl, 'výpadků v období', status.gaps.length);
}

function renderLog(status) {
  // One reverse-chronological stream out of two sources: what arrived
  // (ingestEvents, in-memory, resets on server restart) and what didn't
  // (gaps, derived from the stored timestamps).
  const entries = [
    ...status.ingestEvents.map((event) => ({
      atMs: Date.parse(event.atIso),
      kind: event.inserted === 0 ? 'duplikát' : 'příjem',
      tone: event.inserted === 0 ? 'warn' : 'ok',
      detail:
        `${event.count}× záznam, uloženo ${event.inserted}` +
        (event.duplicates ? `, zahozeno jako duplikát ${event.duplicates}` : '') +
        (event.backfilled ? `, backfill ${event.backfilled}` : '') +
        (event.remoteAddress ? ` · ${event.remoteAddress}` : ''),
    })),
    ...status.gaps.map((gap) => ({
      atMs: Date.parse(gap.toIso),
      kind: 'výpadek',
      tone: 'bad',
      detail: `${gap.seconds} s bez dat (${formatLocalTime(Date.parse(gap.fromIso), { seconds: true })} → ${formatLocalTime(Date.parse(gap.toIso), { seconds: true })})`,
    })),
  ].sort((a, b) => b.atMs - a.atMs);

  logEl.innerHTML = '';
  logEmptyEl.hidden = entries.length > 0;
  for (const entry of entries) {
    const tr = el('tr');
    tr.append(el('td', { text: formatLocalStamp(entry.atMs) }));
    const kind = el('td');
    kind.append(badge(entry.kind, entry.tone));
    tr.append(kind, el('td', { text: entry.detail }));
    logEl.append(tr);
  }
}

function renderRecent(recent) {
  recentEl.innerHTML = '';
  recentEmptyEl.hidden = recent.length > 0;
  for (const reading of recent) {
    const tr = el('tr');
    tr.append(
      el('td', { text: formatLocalTime(Date.parse(reading.capturedAt), { seconds: true }) }),
      el('td', { text: formatLocalTime(Date.parse(reading.receivedAt), { seconds: true }) }),
      el('td', { text: `${formatNumber(reading.windSpeedMs, 2)} m/s` }),
      el('td', { text: `${octantLabel(reading.windDirOctant)} (${reading.windDirOctant})` }),
      el('td', { text: reading.rssiDbm === null ? '—' : `${reading.rssiDbm} dBm` }),
      el('td', { text: reading.clockSynced ? 'ok' : 'ne' }),
      el('td', { text: reading.backfilled ? 'ano' : '—' }),
      el('td', { text: reading.clientId }),
    );
    recentEl.append(tr);
  }
}

function renderRssi(readings) {
  const points = buildRssiPoints(readings);
  rssiEmptyEl.hidden = points.length > 0;

  const data = {
    datasets: [
      {
        label: 'RSSI (dBm)',
        data: points,
        borderColor: ACCENT,
        backgroundColor: ACCENT,
        borderWidth: 2,
        tension: 0.25,
        pointRadius: 0,
        pointHitRadius: 12,
      },
      // Flat reference lines rather than a Chart.js annotation plugin - we
      // vendor exactly one Chart.js file and adding a plugin for two lines
      // is not worth it.
      ...[
        { value: RSSI_GOOD, label: 'dobrý signál (-67)', color: '#4fd48a' },
        { value: RSSI_MARGINAL, label: 'hraniční (-80)', color: '#ffc86b' },
      ].map((line) => ({
        label: line.label,
        data: currentWindow
          ? [
              { x: Date.parse(currentWindow.from), y: line.value },
              { x: Date.parse(currentWindow.to), y: line.value },
            ]
          : [],
        borderColor: line.color,
        borderDash: [4, 4],
        borderWidth: 1,
        pointRadius: 0,
        fill: false,
      })),
    ],
  };

  const options = {
    parsing: false,
    maintainAspectRatio: false,
    animation: false,
    interaction: { mode: 'nearest', axis: 'x', intersect: false },
    scales: {
      x: {
        type: 'linear',
        min: currentWindow ? Date.parse(currentWindow.from) : undefined,
        max: currentWindow ? Date.parse(currentWindow.to) : undefined,
        ticks: {
          color: MUTED,
          maxRotation: 0,
          autoSkipPadding: 24,
          callback: (value) => formatLocalTime(value, { seconds: (currentWindow?.rangeSeconds ?? 3600) <= 900 }),
        },
        grid: { color: GRID },
      },
      y: {
        type: 'linear',
        // RSSI is negative and "less negative is better"; a zero-anchored
        // axis would squash the entire useful range into a sliver.
        suggestedMin: -95,
        suggestedMax: -30,
        title: { display: true, text: 'RSSI (dBm)', color: MUTED },
        ticks: { color: MUTED },
        grid: { color: GRID },
      },
    },
    plugins: {
      legend: { labels: { color: MUTED, usePointStyle: true } },
      tooltip: { callbacks: { title: (items) => formatLocalStamp(items[0].parsed.x) } },
    },
  };

  if (!rssiChart) {
    rssiChart = new Chart(document.getElementById('rssi-chart'), { type: 'line', data, options });
    return;
  }
  rssiChart.data = data;
  rssiChart.options = options;
  rssiChart.update();
}

async function refresh() {
  if (!currentWindow) {
    return;
  }
  const params = new URLSearchParams({ from: currentWindow.from, to: currentWindow.to, limit: '50' });
  const [status, history] = await Promise.all([
    fetch(`/readings/status?${params}`).then((res) => res.json()),
    fetch(
      `/readings/history?${new URLSearchParams({
        from: currentWindow.from,
        to: currentWindow.to,
        bucket: String(currentWindow.bucketSeconds),
      })}`,
    ).then((res) => res.json()),
  ]);

  lastStatus = status;
  renderLatest(status.latest);
  renderTotals(status);
  renderLog(status);
  renderRecent(status.recent);
  renderRssi(history.readings ?? []);
}

const timeRange = mountTimeRange(document.getElementById('timerange'), (nextWindow) => {
  currentWindow = nextWindow;
  refresh();
});

// Keeps the "stáří" line ticking up between refreshes, so a station that has
// gone silent is visibly going stale rather than looking frozen-but-fine.
setInterval(() => {
  if (lastStatus) {
    renderLatest(lastStatus.latest);
  }
}, 1000);

// Same shared-socket pattern as the dashboard: the socket is the "something
// changed" nudge, the REST endpoints stay the source of truth (AD-6).
connectLiveSocket(`${location.protocol === 'https:' ? 'wss:' : 'ws:'}//${location.host}/readings/live`, {
  onOpen: () => timeRange.refresh(),
});

setInterval(() => timeRange.refresh(), 10_000);
