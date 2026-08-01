# Cerebrum

> OpenWolf's learning memory. Updated automatically as the AI learns from interactions.
> Do not edit manually unless correcting an error.
> Last updated: 2026-08-01

## User Preferences

<!-- How the user likes things done. Code style, tools, patterns, communication. -->

## Key Learnings

- **Project:** gustik

## Do-Not-Repeat

<!-- Mistakes made and corrected. Each entry prevents the same mistake recurring. -->
<!-- Format: [YYYY-MM-DD] Description of what went wrong and what to do instead. -->

- [2026-08-01] `.wolf/hooks/post-write.js` exits early for EVERY path under `.wolf/` (self-referential-write guard) — it never pushes `.wolf/*` writes into `session.files_written`. Any Stop-hook check that inspects `files_written` to ask "was `.wolf/X` updated this session?" is structurally unsatisfiable (see bug-002, bug-004 in buglog.json — two independent instances of this exact pattern). If adding a new such check, check the target file's own `mtime` via `fs.statSync` against `session.started` instead (the pattern `checkStatusFreshness()` already uses correctly), never `files_written`.

## Decision Log

<!-- Significant technical decisions with rationale. Why X was chosen over Y. -->

- **[2026-08-01] Architecture spine finalized (Fast path + reviewer gate).** Backend: Node.js 24 + Fastify 5.11.x + better-sqlite3 (SQLite), not the addendum-mentioned FastAPI/Python — picked to reuse the devcontainer's already-set-up Node runtime rather than add a second language on a 16-day timeline. Firmware: C++/Arduino framework via PlatformIO (stable 2.x ESP32 core, not 3.x — PlatformIO support for 3.x is unsettled). Dashboard: vanilla JS + Chart.js, no frontend build step, served as static assets by Fastify — one Docker image, one process. Full rationale + reviewer-gate findings: `_bmad-output/planning-artifacts/architecture/architecture-gustik-2026-08-01/.memlog.md`.
- **[2026-08-01] HMC5883L sourcing risk flagged.** Genuine HMC5883L magnetometer chips are discontinued; GY-271/273 modules sold today mostly ship QMC5883L die under HMC5883L silkscreen (different I2C address/registers). Architecture spine pins a dual-chip-compatible library (e.g. DFRobot_QMC5883) instead of a HMC5883L-only one — but the actual part Guru/Mlok source should be checked against this before ordering.
- **[2026-08-01] Epics/stories breakdown (`bmad-create-epics-and-stories`).** 5 epics, 18 stories, in `_bmad-output/planning-artifacts/epics.md`. Epic order follows PRD's literal priority ("funkční živé čtení > historický graf > vyladěná mechanika") as a strict sequence: Epic 1 walking-skeleton live path (FR-1,2,3,7,9,10,11) stands alone and fully functional even without Epic 2; Epic 2 layers connectivity resilience (buffer/backfill/LED/RSSI, FR-4,5,6) on top; Epic 3 historical graph (FR-8); Epic 4 zero-touch provisioning (FR-14,15); Epic 5 power/mechanical (FR-12,13, hardware-only, no software). Epic 1 and 2 both touch `firmware/src/transmit` + `backend/src/ingest` — this was a deliberate file-churn call, not an oversight: kept as two epics because it mirrors the PRD's own staged priority (prove live transmission works before adding resilience), not because of an arbitrary technical split.

## Key Learnings (session-process notes)

- **BMAD skill workflows assume a live interactive user at every menu ("halt and wait for [C] Continue").** When one of these skills is run inside an autonomous subagent invocation (Agent tool, no attached interactive user), there is no way to literally pause across turns — the skill's step files must be read and followed for *content/structure* discipline (extraction rigor, dependency rules, coverage validation), but the halt-for-approval menus have to be resolved using whatever brief/context the invoking session provided, standing in for the user's answers. Always flag this explicitly in the output (both in the artifact itself and in STATUS.md) so the real user knows no live back-and-forth happened and should spot-check the result once.
