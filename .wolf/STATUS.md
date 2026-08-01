# STATUS — gustik

> Single source of truth for resuming work. Read this FIRST when starting a session.
> Update this file at the end of every work phase so the next `/clear` resumes in 1 read.
> Last updated: 2026-08-01

---

## ✅ Done

- **PRD finalized** (BMAD `bmad-prd`, Fast path): `_bmad-output/planning-artifacts/prds/prd-gustik-2026-08-01/prd.md` (status: final) + `addendum.md`. 15 FRs across 6 features (wind measurement, connectivity/resilience, dashboard, power, mechanical install, setup/provisioning), 4 UJs, 7 SMs, reviewer gate passed (rubric review + input reconciliation against `brief.md`/`addendum.md`), editorial polish applied.
- Target regatta confirmed: Plachetní soustředění na Nechanicích (YC4PVS), start 2026-08-17 — 16 days out from PRD date.
- OpenWolf hook bug fixed: `.wolf/hooks/symbol-extractor.js` was missing from the repo (install gap in commit `f532148`), causing every PostToolUse:write hook to throw `ERR_MODULE_NOT_FOUND`. Copied from the globally installed `openwolf@2.0.1` package; verified hook now exits 0. File is untracked in git — commit it if you want the fix preserved.

---

## 🚀 Next phase

**Goal:** Continue the BMAD planning chain from the finalized PRD — next standard steps are UX spec and/or architecture, per the PRD's own handoff note.

### Options (user picks, not yet decided)
1. `bmad-ux` — plan UX patterns for the dashboard (4 UJs already capture user-facing flows; may be light-touch given hobby stakes).
2. `bmad-architecture` — produce the architecture spine (backend stack choice, firmware structure) — likely most load-bearing next step since PRD leaves backend framework/DB/frontend tech explicitly open.
3. `bmad-create-epics-and-stories` — skip straight to breaking the PRD into epics/stories if architecture-level decisions can be made informally given the tight timeline.

### Open decisions carried from PRD (`prd.md` §9, `addendum.md`)
- Backend framework/database/frontend tech (deployment target already fixed: Docker/Compose behind existing Caddy reverse proxy).
- Magnetometer physical placement (mast-top near sensors vs. near ESP on deck — I2C bus-length trade-off).
- Whether a physical shore/mobile-hotspot switch is needed (depends on Wi-Fi auto-fallback reliability, untested).
- Real-world Wi-Fi range from shore at Nechanice (untested; FR-6 RSSI logging exists to measure it on-site).
- Local buffer capacity (4h target is a placeholder pending ESP32 flash capacity check).

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
