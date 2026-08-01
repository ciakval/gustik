import { test } from 'node:test';
import assert from 'node:assert/strict';
import { WebSocket } from 'ws';
import { buildApp } from '../src/app.js';
import { connectLiveSocket } from '../src/static/live-socket.js';

function withTimeout(promise, ms, label) {
  return Promise.race([
    promise,
    new Promise((_, reject) => setTimeout(() => reject(new Error(`timed out: ${label}`)), ms)),
  ]);
}

async function withLiveApp(fn) {
  const app = buildApp({ dbPath: ':memory:', ingestToken: 'secret-token' });
  await app.listen({ port: 0, host: '127.0.0.1' });
  const { port } = app.server.address();
  try {
    await fn(app, `ws://127.0.0.1:${port}/readings/live`);
  } finally {
    await app.close();
  }
}

test('connectLiveSocket calls onOpen when the connection is established', async () => {
  await withLiveApp(async (app, url) => {
    let opened = false;
    let controller;
    const openedPromise = new Promise((resolve) => {
      controller = connectLiveSocket(url, { WebSocketImpl: WebSocket, onOpen: () => { opened = true; resolve(); } });
    });
    try {
      await withTimeout(openedPromise, 2000, 'onOpen');
      assert.equal(opened, true);
    } finally {
      controller.close();
    }
  });
});

test('connectLiveSocket calls onMessage with the parsed JSON payload', async () => {
  await withLiveApp(async (app, url) => {
    let controller;
    const received = new Promise((resolve) => {
      controller = connectLiveSocket(url, { WebSocketImpl: WebSocket, onMessage: resolve });
    });
    try {
      // wait for the connection to actually be registered server-side before
      // broadcasting, otherwise the message would have nowhere to go
      await new Promise((resolve) => setTimeout(resolve, 100));
      for (const socket of app.wsClients) {
        socket.send(JSON.stringify({ event: 'history-changed' }));
      }
      const message = await withTimeout(received, 2000, 'onMessage');
      assert.deepEqual(message, { event: 'history-changed' });
    } finally {
      controller.close();
    }
  });
});

test('connectLiveSocket AC2: reconnects and calls onOpen again after the connection is closed (AD-6 resync-on-reconnect)', async () => {
  await withLiveApp(async (app, url) => {
    let openCount = 0;
    let firstOpenResolve;
    const firstOpen = new Promise((resolve) => { firstOpenResolve = resolve; });
    let secondOpenResolve;
    const secondOpen = new Promise((resolve) => { secondOpenResolve = resolve; });

    const controller = connectLiveSocket(url, {
      WebSocketImpl: WebSocket,
      reconnectDelayMs: 10,
      onOpen: () => {
        openCount++;
        if (openCount === 1) firstOpenResolve();
        if (openCount === 2) secondOpenResolve();
      },
    });

    try {
      await withTimeout(firstOpen, 2000, 'first open');
      // simulate the connection dropping (network hiccup, server restart) -
      // force-close every server-side socket
      for (const socket of app.wsClients) {
        socket.close();
      }
      await withTimeout(secondOpen, 2000, 'reconnect open');
      assert.equal(openCount, 2);
    } finally {
      controller.close();
    }
  });
});
