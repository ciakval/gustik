import { getLatest } from '../store/readings.js';

export function registerServeRoutes(fastify) {
  fastify.get('/readings/latest', async () => {
    return { reading: getLatest(fastify.db) };
  });

  fastify.get('/readings/live', { websocket: true }, (socket) => {
    fastify.wsClients.add(socket);
    socket.on('close', () => fastify.wsClients.delete(socket));
  });
}

function broadcast(fastify, message) {
  const payload = JSON.stringify(message);
  for (const socket of fastify.wsClients) {
    if (socket.readyState === socket.OPEN) {
      socket.send(payload);
    }
  }
}

export function broadcastReading(fastify, reading) {
  broadcast(fastify, reading);
}

// Bare event (AD-9) - tells listeners to refetch /readings/history rather
// than carrying any payload itself. Harmless if nothing is listening yet
// (Story 2.3 AC3 - Epic 3 is the first real consumer).
export function broadcastHistoryChanged(fastify) {
  broadcast(fastify, { event: 'history-changed' });
}
