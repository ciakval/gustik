// Single shared WS connection lifecycle for the dashboard - both the live
// reading view (dashboard.js) and the history chart (history-chart.js)
// subscribe to the same socket instead of each opening their own. Handles
// reconnect-with-retry itself (Story 3.3 AC2/AD-6): every (re)connect fires
// onOpen, so subscribers should treat "just connected" as "go resync via
// REST", not just "handle the first connection".
//
// WebSocketImpl is injectable so this can be unit tested against a real
// server with the `ws` package (Node's native global WebSocket client has a
// known issue talking to a `ws`-based server in this environment - see
// buglog bug-007's related note) while defaulting to the browser's global
// WebSocket in production.
//
// Returns a controller with close() - stops the reconnect loop and closes
// the current connection. Without this, a torn-down server (or a test's
// app.close()) leaves an orphaned reconnect timer retrying forever.
export function connectLiveSocket(url, { onMessage, onOpen, WebSocketImpl = globalThis.WebSocket, reconnectDelayMs = 2000 } = {}) {
  let stopped = false;
  let currentWs = null;
  let reconnectTimer = null;

  function connect() {
    const ws = new WebSocketImpl(url);
    currentWs = ws;
    ws.addEventListener('open', () => onOpen?.());
    ws.addEventListener('message', (event) => onMessage?.(JSON.parse(event.data)));
    ws.addEventListener('close', () => {
      if (!stopped) {
        reconnectTimer = setTimeout(connect, reconnectDelayMs);
      }
    });
  }
  connect();

  return {
    close() {
      stopped = true;
      if (reconnectTimer) {
        clearTimeout(reconnectTimer);
      }
      currentWs?.close();
    },
  };
}
