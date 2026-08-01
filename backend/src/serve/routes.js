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

export function broadcastReading(fastify, reading) {
  const message = JSON.stringify(reading);
  for (const socket of fastify.wsClients) {
    if (socket.readyState === socket.OPEN) {
      socket.send(message);
    }
  }
}
