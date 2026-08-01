# STATUS — gustik

> Single source of truth for resuming work. Read this FIRST when starting a session.
> Update this file at the end of every work phase so the next `/clear` resumes in 1 read.
> Last updated: 2026-08-01

---

## ✅ Done

- **PRD finalized** (BMAD `bmad-prd`, Fast path): `_bmad-output/planning-artifacts/prds/prd-gustik-2026-08-01/prd.md` (status: final) + `addendum.md`. 15 FRs across 6 features (wind measurement, connectivity/resilience, dashboard, power, mechanical install, setup/provisioning), 4 UJs, 7 SMs, reviewer gate passed (rubric review + input reconciliation against `brief.md`/`addendum.md`), editorial polish applied.
- Target regatta confirmed: Plachetní soustředění na Nechanicích (YC4PVS), start 2026-08-17 — 16 days out from PRD date.
- OpenWolf hook bug fixed: `.wolf/hooks/symbol-extractor.js` was missing from the repo (install gap in commit `f532148`), causing every PostToolUse:write hook to throw `ERR_MODULE_NOT_FOUND`. Fixed locally (committed) and confirmed live-and-unreported on `cytostack/openwolf` upstream too — PR ready, parked pending GH token permissions (see `.wolf/buglog.json` bug-001, memory).
- Devcontainer host/container home-path bind-mount fix committed (`136bc92`) — see `.devcontainer/devcontainer.json`/`post-create.sh`.
- **Architecture spine finalized** (BMAD `bmad-architecture`, Fast path + full reviewer gate — rubric + version/currency + adversarial-divergence, all as parallel subagents, all findings applied): `_bmad-output/planning-artifacts/architecture/architecture-gustik-2026-08-01/ARCHITECTURE-SPINE.md` (status: final). Paradigm: layered pipeline (Sense→Correct→Buffer/Transmit→Ingest→Store→Serve). Key picks: Node.js 24 + Fastify 5.11.x + better-sqlite3 backend (not FastAPI — reuses devcontainer's existing Node setup); PlatformIO + Arduino ESP32 core 2.x firmware; vanilla JS + Chart.js dashboard, no frontend build step; single Docker image behind existing Caddy. 10 ADs covering the write boundary, ordering, correction locus, direction encoding, WS/REST authority split, dependency direction, payload/idempotency, ingest auth, and clock sync. Reviewer gate caught and fixed a real hardware risk (genuine HMC5883L chips discontinued — spine now pins a QMC5883L-compatible library instead) plus several cross-unit divergence gaps (backfill payload shape, WS message shape, RSSI read-path ownership, Docker base image for better-sqlite3). Full decision rationale in the run's `.memlog.md`; three full reviews under `reviews/`.

---

## 🚀 Next phase

**Goal:** Break the finalized PRD + architecture spine into epics/stories.

**Recommended:** `bmad-create-epics-and-stories`, using both `prd.md` and `ARCHITECTURE-SPINE.md` as inputs (spine's Capability→Architecture Map already bridges PRD features to components/ADs). The skill's own Finalize step also suggested `bmad-spec` (adopt the spine as a spec companion) as a lead-in — worth considering if per-story machine-readable contracts (esp. the AD-8 payload shape / AD-9 WS shape / readings schema) would help solo-dev consistency across sessions more than jumping straight to epics.

### Open decisions carried from PRD/architecture (deferred, not blocking)
- Magnetometer physical placement (mast-top near sensors vs. near ESP on deck — I2C bus-length trade-off) — mechanical/electrical phase, not architecture.
- Whether a physical shore/mobile-hotspot switch is needed (depends on Wi-Fi auto-fallback reliability, untested).
- Real-world Wi-Fi range from shore at Nechanice (untested; FR-6 RSSI logging + spine's serve-side exposure now make this measurable post-event).
- Local buffer capacity (4h target) and exact sampling interval (~2–5s) — pending ESP32 flash capacity check at firmware-implementation time.
- **HMC5883L sourcing**: confirm the actual purchased magnetometer module's chip (HMC5883L vs. QMC5883L clone) before wiring the firmware library — see cerebrum Decision Log.

---

## 📁 Active architecture

- **Stack:** _<frameworks, libraries, runtime>_
- **Key tables / modules:** _<list>_
- **Patterns:** _<conventions enforced project-wide>_

---

## ⚠️ External blockers (don't block coding)

- _<env vars, secrets, external accounts, manual steps>_

---

## 🔧 Useful commands

```bash
# add the most-used commands here so the next session has them ready
```

---

## 📚 References (read IF needed)

- `.wolf/cerebrum.md` — User Preferences + Do-Not-Repeat + Decision Log
- `.wolf/anatomy.md` — token-efficient file index
- `.wolf/buglog.json` — known bugs + fixes
