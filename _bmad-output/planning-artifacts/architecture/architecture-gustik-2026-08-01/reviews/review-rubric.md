---
title: Architecture Spine Review — Gustik
reviews: ../ARCHITECTURE-SPINE.md
against: rubric (good-spine checklist)
created: 2026-08-01
---

# Review: ARCHITECTURE-SPINE.md (Gustik, 2026-08-01)

Reviewed against the good-spine checklist. Sources cross-checked: `prd.md`, `addendum.md`, `.memlog.md` (rationale trail).

## Overall verdict

The spine correctly identifies and locks down the real cross-unit divergence points for the Sense→Correct→Buffer/Transmit→Ingest→Store→Serve pipeline (write boundary, ordering key, correction locus, direction encoding, WS/REST authority split, dependency direction), and every FR-1..FR-15 has a home in the Capability Map — but it is silent on the entire operational/reliability envelope for a single-process system that a live safety decision depends on, and it leaves at least one genuine cross-epic contract point (backfill payload shape) undecided where firmware and backend teams (even if the same solo dev, at different points in time) could reasonably diverge.

## 1. Does it fix the real divergence points for the level below (epics/stories)?

**Covered well:**
- Write boundary (AD-1), ordering key (AD-2), correction locus (AD-4), direction encoding (AD-5), WS-vs-REST authority (AD-6), dependency direction (AD-7) are exactly the kind of decisions that, left unstated, would let a firmware epic and a backend epic quietly disagree. Good altitude discipline — these are picked because PRD FR wording alone doesn't nail them down (e.g., FR-4's backfill requirement doesn't by itself imply "sort by captured_at, not insertion order" — that's a genuine architectural addition).
- Naming/casing convention (snake_case DB ↔ camelCase JSON, conversion confined to one layer) preempts a classic two-sided bikeshed.

**Missed — real divergence points left undecided:**

1. **Backfill payload shape (single reading vs. batch) is not decided.** AD-1's rule is phrased around "jeden HTTP POST endpoint" (singular reading, implicitly). But FR-4 requires the Stanice to buffer up to ~4h of readings and backfill them on reconnect — at a 2–5s sampling interval that's up to ~7,200 buffered records. Whether the ingest endpoint accepts one reading per POST (replayed thousands of times sequentially) or an array/batch payload is a real contract both the firmware `transmit/` epic and the backend `ingest/` epic need to agree on independently, and nothing in the spine picks one. This is precisely the kind of thing a spine should fix and currently doesn't.

2. **Validation/error-response contract at the ingest boundary is unstated.** The pipeline table says Ingest does "validace" but no rule says what happens on a malformed/out-of-range payload (reject with 4xx and drop record? log and 200 anyway to avoid blocking firmware's simple retry loop?). Given firmware "never blocks or halts sampling on send failure" (Consistency Conventions), the two sides need a shared understanding of what counts as a send failure worth buffering-and-retrying vs. a validation failure that should be dropped. Left to story-level improvisation.

## 2. Is every AD's Rule enforceable and does it actually prevent its stated divergence?

Six of seven hold up. One is undermined by an unstated dependency:

- **AD-2 ("Pořadí podle času měření, ne příjmu") is not fully self-sufficient.** Its stated purpose is to prevent a post-outage backfill burst from scrambling history order, and its mechanism is "sort by `captured_at`, the device's own clock at sample time." But nothing in the spine (or PRD/addendum) establishes how the ESP32 obtains and maintains an accurate wall clock. ESP32 has no battery-backed RTC by default; the Consistency Conventions require `captured_at` on the wire as an **ISO-8601 UTC string**, which means the device must NTP-sync at some point — and the natural moment to NTP-sync (on Wi-Fi connect) is exactly the moment that's least reliable during/after the outages FR-4 exists to survive. If the device boots with an unsynced or drifted clock and starts buffering before it has ever synced, `captured_at` values for the whole outage window could be wrong or non-monotonic with the rest of the dataset, and AD-2's ordering guarantee silently fails to prevent the exact divergence it names. This should either be a rule (e.g., "Stanice buffers relative device-uptime and re-stamps to wall-clock only after first NTP sync" or similar) or an explicit open question — right now it's neither.

The other six (AD-1, AD-3, AD-4, AD-5, AD-6, AD-7) have rules that are mechanically checkable (schema shape, single-process/single-file, sort key, dependency direction) and genuinely prevent what they claim to prevent.

## 3. Could anything under Deferred let two independently-built units diverge?

No — the seven deferred items (magnetometer placement, buffer capacity, sampling interval, physical shore/mobile switch, multi-station, OTA, tilt/IMU) are each either purely mechanical/electrical (no software contract implications) or scoped to a single unit (firmware-internal tuning) with no cross-unit interface exposed. None of them is load-bearing across the Stanice/Backend/Dashboard boundary. This section of the spine is sound.

## 4. Named tech verified-current / suspicious items

- Node 24 (Active LTS by mid-2026 per the even-release-year LTS cadence), Fastify 5.11.x, better-sqlite3 13.0.x, Chart.js 4.5.x — plausible given normal release cadences; nothing here reads as fabricated or internally inconsistent.
- **Flag:** the Stack table's justification for staying on `framework-arduinoespressif32` "stabilní 2.x větev" is carried from `.memlog.md`'s claim that "ESP32 Arduino Core 3.0.0 exists but PlatformIO support is unsettled as of 2026." This is worth re-verifying before firmware work starts — Arduino-ESP32 core 3.x had reached solid PlatformIO support (including via the community `pioarduino` platform fork) well before 2026 in most trackers. If that claim is stale, the project may be pinning to an older core branch than necessary for no real benefit, which matters here because core 3.x has materially better power-management APIs — directly relevant to FR-12 (8h+ on a powerbank, Wi-Fi radio is the dominant power draw per §11.1). Not fatal, but the "unsettled" framing should be spot-checked, not taken as settled fact.

## 5. PRD/addendum FR coverage

All FR-1..FR-15 are named in `binds:` and appear in the Capability Map. FR-12/FR-13 are correctly marked "mimo softwarovou architekturu" rather than silently dropped — good practice, not a gap.

One weak link: the Capability Map row for **FR-14/FR-15** cites "Governed by: Consistency Conventions (State & cross-cutting)" — but that table row only actually addresses Wi-Fi credential storage and firmware error-handling behavior. It says nothing about FR-15 (published written provisioning instructions: where they live, how they're reachable without a dev environment, that shore-hotspot SSID/password must be kept in sync with what's hardcoded in firmware). FR-15 is claimed as "governed" but is, in substance, ungoverned by any rule or convention in the spine. Low severity (it's a docs/content requirement, not a runtime contract), but the citation is misleading as written.

## 6. Dimensions the feature altitude owns: decided / deferred / open question — gap check

This is the checklist item the spine fares worst on.

- **Deployment target**: decided (adopted from addendum — Docker/Compose behind existing Caddy, self-hosted). Fine.
- **Environments**: not explicitly addressed, but for a single solo-built station shipping once to one event, "there is only prod" is a defensible implicit default and low-risk to leave unstated.
- **Infra/provider strategy**: decided (existing server, addendum-adopted). Fine.
- **Operations — silent, and this is the most significant finding in this review.** The spine gives zero treatment to:
  - **Process resilience**: AD-3 deliberately commits to "one process, one SQLite file" — a legitimate simplicity call for this scale, but that also makes the Fastify process a single point of failure for the *entire system* during an 8-hour live event where a human is making capsize-risk safety calls off its data. Nothing states a restart policy (e.g., `restart: unless-stopped`/`on-failure` in docker-compose), a health check, or what "recovery" looks like if the container dies mid-regatta. AD-6 (REST is source of truth, dashboard reconciles on reconnect) mitigates *WebSocket* drops, but does nothing for the underlying process itself going down.
  - **Data durability**: the structural-seed diagram shows the SQLite file as a "mount volume," which implies persistence-across-restarts was considered, but this is never elevated to a stated rule, and there's no mention of backup (even a trivial nightly copy of a single SQLite file) despite the data being the entire evidentiary record FR-8/FR-15/SM-4/SM-7 depend on.
  - **Observability**: no mention of logging or any way to notice the backend is down before a human on the boat notices missing dashboard data. Given FR-11's dashboard staleness indicator exists precisely to surface this to end users, the operator side (Mlok/whoever's on call) has no equivalent signal.
  
  Given this is explicitly named in the review rubric as a dimension that must be decided/deferred/or flagged as an open question at feature altitude — and the PRD itself (§12) frames Gustik as safety-decision support, and §11.3 explicitly calls out "provozní a spolehlivostní požadavky" as a first-class concern — leaving operations completely unaddressed (not even as a deferred item or open question) is a real gap, not a reasonable omission. A one-line AD ("Docker restart policy: always; SQLite file backed up via X; no other observability in v1 — acceptable given single-day, human-monitored deployment") would have closed this cheaply.
- **Security/access control of the ingest endpoint**: FR-9 explicitly decides the *dashboard* (read side) is public/unauthenticated by design. Nothing anywhere — PRD, addendum, or spine — decides the same question for the *write* side. `POST /readings` sits behind the same public Caddy proxy as everything else (per the structural-seed diagram), with AD-1 establishing it as the *only* write path into the system of record that a human safety decision is read from. If that endpoint is unauthenticated (as the current design implies — no API key/shared-secret is mentioned anywhere), anyone who finds or scans the URL can inject fabricated wind readings that a regatta official might act on. This is squarely an architectural decision (does the endpoint require a shared secret header set in both firmware config and backend env?) that the spine should decide, defer explicitly, or at least log as an open question — it currently does none of the three.

## Summary of findings (ranked)

1. **[High] Operations/resilience dimension is entirely unaddressed** — no restart policy, health check, backup, or observability decision for the single process/single SQLite file the whole live event depends on (AD-3's simplicity is fine; its operational consequence is not covered anywhere).
2. **[High] Ingest endpoint (`POST /readings`) has no stated authentication/access-control decision**, despite being the sole write path (AD-1) into a dataset that drives a real safety decision, exposed on the public internet via the existing Caddy proxy.
3. **[Medium] Backfill payload shape (single-record vs. batch POST) is undecided**, even though FR-4's buffer (up to ~4h, thousands of records at the stated sampling interval) makes this a real contract point between the firmware `transmit/` epic and backend `ingest/` epic.
4. **[Medium] AD-2's ordering guarantee assumes an accurate device clock that nothing establishes** — no NTP-sync rule or fallback is specified, and the outage windows AD-2 is designed to survive are exactly when clock sync is least reliable.
5. **[Low] Two documentation nits**: (a) the Arduino-ESP32 2.x-vs-3.x PlatformIO-maturity claim driving the firmware framework pin looks stale/worth re-checking before firmware work starts, given 3.x's better power-management APIs are directly relevant to the FR-12 battery-life target; (b) the Capability Map's "Governed by: Consistency Conventions" citation for FR-15 doesn't actually correspond to any rule about published instructions in that table.
