import { buildApp } from './app.js';

const port = Number(process.env.PORT ?? 3000);
const dbPath = process.env.DB_PATH ?? './data/gustik.sqlite';
const ingestToken = process.env.INGEST_TOKEN;

if (!ingestToken) {
  console.error('INGEST_TOKEN env var is required');
  process.exit(1);
}

const app = buildApp({ dbPath, ingestToken });

app.listen({ port, host: '0.0.0.0' }).catch((err) => {
  app.log.error(err);
  process.exit(1);
});
