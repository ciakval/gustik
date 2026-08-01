# STATUS — gustik

> Single source of truth for resuming work. Read this FIRST when starting a session.
> Update this file at the end of every work phase so the next `/clear` resumes in 1 read.
> Last updated: 2026-08-01

---

## ✅ Done

- **Epics and stories created** (BMAD `bmad-create-epics-and-stories`, autonomous run — no live interactive elicitation, ran end-to-end from the PRD/architecture spine per the task brief's timeline/priority framing): `_bmad-output/planning-artifacts/epics.md`. 5 epics, 18 stories, all 15 FRs covered (verified — see doc's own "Validace pokrytí" section). Epics ordered exactly per PRD's stated priority "funkční živé čtení > historický graf > vyladěná mechanika": Epic 1 Základní živý přenos větru (walking skeleton, FR-1/2/3/7/9/10/11, 6 stories) → Epic 2 Odolnost spojení a diagnostika (buffer/backfill/LED/RSSI, FR-4/5/6, 5 stories) → Epic 3 Historický graf (FR-8, 3 stories) → Epic 4 Bezobslužné zprovoznění (FR-14/15, 2 stories) → Epic 5 Výdrž napájení a mechanická instalace (FR-12/13, 2 stories, hardware-only). Epic 1 stands alone (FR-3 already includes auto-reconnect, so live reading works without Epic 2's resilience layer). No story has a forward dependency within its epic. File churn on `firmware/src/transmit` + `backend/src/ingest` (shared by Epic 1 and Epic 2) assessed and consciously not consolidated — matches the PRD's own staged priority (prove live transmission first, add resilience second).
- **PRD finalized** (BMAD `bmad-prd`, Fast path): `_bmad-output/planning-artifacts/prds/prd-gustik-2026-08-01/prd.md` (status: final) + `addendum.md`. 15 FRs across 6 features (wind measurement, connectivity/resilience, dashboard, power, mechanical install, setup/provisioning), 4 UJs, 7 SMs, reviewer gate passed (rubric review + input reconciliation against `brief.md`/`addendum.md`), editorial polish applied.
- Target regatta confirmed: Plachetní soustředění na Nechanicích (YC4PVS), start 2026-08-17 — 16 days out from PRD date.
- OpenWolf hook bug fixed: `.wolf/hooks/symbol-extractor.js` was missing from the repo (install gap in commit `f532148`), causing every PostToolUse:write hook to throw `ERR_MODULE_NOT_FOUND`. Fixed locally (committed) and confirmed live-and-unreported on `cytostack/openwolf` upstream too — PR ready, parked pending GH token permissions (see `.wolf/buglog.json` bug-001, memory).
- Devcontainer host/container home-path bind-mount fix committed (`136bc92`) — see `.devcontainer/devcontainer.json`/`post-create.sh`.
- **Architecture spine finalized** (BMAD `bmad-architecture`, Fast path + full reviewer gate — rubric + version/currency + adversarial-divergence, all as parallel subagents, all findings applied): `_bmad-output/planning-artifacts/architecture/architecture-gustik-2026-08-01/ARCHITECTURE-SPINE.md` (status: final). Paradigm: layered pipeline (Sense→Correct→Buffer/Transmit→Ingest→Store→Serve). Key picks: Node.js 24 + Fastify 5.11.x + better-sqlite3 backend (not FastAPI — reuses devcontainer's existing Node setup); PlatformIO + Arduino ESP32 core 2.x firmware; vanilla JS + Chart.js dashboard, no frontend build step; single Docker image behind existing Caddy. 10 ADs covering the write boundary, ordering, correction locus, direction encoding, WS/REST authority split, dependency direction, payload/idempotency, ingest auth, and clock sync. Reviewer gate caught and fixed a real hardware risk (genuine HMC5883L chips discontinued — spine now pins a QMC5883L-compatible library instead) plus several cross-unit divergence gaps (backfill payload shape, WS message shape, RSSI read-path ownership, Docker base image for better-sqlite3). Full decision rationale in the run's `.memlog.md`; three full reviews under `reviews/`.

---

## 🚀 Next phase

**Goal:** Start implementation on Epic 1 (walking skeleton) — Story 1.1 or 1.3 first.

**Recommended:** `bmad-create-story` for Story 1.1 (Měření rychlosti větru) or Story 1.3 (Ingest endpoint — this one has no hardware dependency and is fully implementable/testable in the devcontainer today, so it's the lowest-friction starting point if Mlok wants to validate the backend skeleton before hardware is wired up), then `bmad-dev-story` to implement it. This session's task brief flagged that `.wolf/STATUS.md`/anatomy.md still describe the repo as having no application code — the first story that lands should also trigger updating CLAUDE.md's "Project state" section and this file's "📁 Active architecture" block (stack/tables/patterns are currently placeholders) since real code will finally exist.

**Also worth considering (not blocking):** `bmad-sprint-planning` first, if Mlok wants a tracked sprint-status.yaml across all 18 stories before diving into story 1 — optional given this is a solo 16-day project, may be overhead vs. just working the epics.md list top to bottom.

### Open decisions carried from PRD/architecture (deferred, not blocking)
- Magnetometer physical placement (mast-top near sensors vs. near ESP on deck — I2C bus-length trade-off) — mechanical/electrical phase, not architecture. Now landed as an explicit AC in epics.md Story 5.2 (must be resolved and recorded, not re-opened, during that story).
- Whether a physical shore/mobile-hotspot switch is needed (depends on Wi-Fi auto-fallback reliability, untested) — now an explicit open note in epics.md Story 4.1.
- Real-world Wi-Fi range from shore at Nechanice (untested; FR-6 RSSI logging + spine's serve-side exposure now make this measurable post-event).
- Local buffer capacity (4h target) and exact sampling interval (~2–5s) — pending ESP32 flash capacity check at firmware-implementation time (Story 2.1/1.1).
- **HMC5883L sourcing**: confirm the actual purchased magnetometer module's chip (HMC5883L vs. QMC5883L clone) before wiring the firmware library — see cerebrum Decision Log. Relevant at Story 1.2 implementation time.
- **New from this run:** epics.md was produced as a fully autonomous pass (no live interactive user was attached to answer the skill's own menu prompts — the task brief's timeline/priority framing was used as the operative input instead). Mlok should skim the Epic List and FR Coverage Map in `_bmad-output/planning-artifacts/epics.md` once, specifically checking: (1) the Epic 1/Epic 2 split (live transmission first, resilience/buffer second) matches his intent, and (2) Epic ordering 3→4→5 (graph, then provisioning, then power/mechanical) is acceptable — PRD's priority statement only explicitly ranks live-reading > graph > mechanics, so provisioning (Epic 4) and connectivity resilience (Epic 2)'s exact placement in that ranking was this run's own inference, not literally stated in the PRD.

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
