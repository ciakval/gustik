FROM node:24-bookworm-slim

# better-sqlite3 ships prebuilt binaries for common targets, but falls back to
# compiling from source (node-gyp) when no prebuilt matches — keep build
# tools available so that fallback doesn't break the image build.
RUN apt-get update \
    && apt-get install -y --no-install-recommends python3 make g++ \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY package.json package-lock.json ./
RUN npm ci --omit=dev

COPY src/ src/

RUN mkdir -p data

ENV NODE_ENV=production
EXPOSE 3000

CMD ["node", "src/index.js"]
