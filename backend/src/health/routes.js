export function registerHealthRoutes(fastify) {
  fastify.get('/health', async (request, reply) => {
    // db.prepare throws if the SQLite file can't be opened/queried
    fastify.db.prepare('SELECT 1').get();
    return reply.code(200).send({ status: 'ok' });
  });
}
