---
title: Adversarial Review — Gustik Architecture Spine
target: ../ARCHITECTURE-SPINE.md
reviewer: Claude (adversarial pass)
date: 2026-08-01
method: construct pairs of independently-buildable units, one level below the spine, that individually obey every AD to the letter, and show they can still ship incompatible
---

# Adversarial Review — Gustik Architecture Spine

## Verdict

The spine's *structural* invariants (single write path, one process/one store, correction-on-device, octant-not-degrees, unidirectional dependency) are tight and hard to violate by accident — AD-1 through AD-7 are genuinely load-bearing. But the spine only constrains **topology**, not **payload/message shape**: nowhere does it pin the exact JSON contract for `POST /readings`, the WebSocket message shape, the exact `readings` column set/types/nullability, or the REST query/response contract for `/readings/history`. Every finding below is a pair of units that each satisfy every AD literally, built by "different people" (firmware vs. backend, or backend vs. dashboard), that still fail to interoperate — because the spine describes *who owns which side of a boundary* but not *what crosses it*.

## Method

For each boundary in the spine (Stanice→Backend, Backend→Dashboard, Backend↔SQLite), I picked two units one level below the spine — roughly epic-sized, each traceable to specific FRs and each assignable to the folder the Capability→Architecture Map already names — and asked: could a developer building unit A, reading only the spine (not unit B's code), make a reasonable, AD-compliant choice that unit B's developer, also reading only the spine, would reasonably make differently? If yes, that's a hole.

---

## Finding 1 (Critical) — `POST /readings` has a topology (AD-1) but no payload contract

**Units:**
- Unit A — `firmware/src/transmit` (FR-3 send, FR-4 buffer/backfill, FR-6 RSSI attach)
- Unit B — `backend/src/ingest` (FR-3/FR-4/FR-6 receive + validate)

**What the spine actually pins:** AD-1 fixes that there is exactly *one* endpoint and nothing else writes rows. AD-2 fixes that every record carries `captured_at` separate from `received_at`, and that ordering is always by `captured_at`. AD-4 fixes that the payload contains only finished `wind_speed_ms` + octant, never raw sensor/magnetometer data. The Naming convention row fixes `camelCase` on the wire and `snake_case` in SQLite. That is the entire contract surface.

**What is left open, and how two AD-compliant builds diverge:**

1. **Envelope shape during backfill.** FR-4 requires that after reconnect, buffered data is "doplní do historie ... ve správném časovém pořadí" (backfilled in correct time order). AD-1 only requires *one endpoint*; it says nothing about whether a request body is a single reading object or a batch. A firmware dev optimizing for "don't hammer a flaky reconnect with hundreds of sequential HTTPS round-trips" reasonably implements backfill as `POST /readings` with a JSON *array* body — still "the one endpoint," still compliant with AD-1's letter. A backend dev, reading the same spine, reasonably implements `ingest/` to `JSON.parse(body)` into a single object matching `{capturedAt, windSpeedMs, windDirOctant, rssiDbm}` and validates it as one record — because AD-4's language ("Backend přijímá vždy jen hotové `wind_speed_ms` a 8-oktantový směr") and the Capability Map's singular framing read as one-reading-per-call. Neither party violated an AD. Result: backfill either 400s on every reconnect (silently defeating FR-4, the exact scenario AD-1/AD-2 exist to protect) or someone discovers the mismatch at integration time under deadline pressure.

2. **Idempotency / duplicate inserts on retry.** FR-3 requires the Stanice to retry on failed send "aniž by vyžadovala restart nebo zásah obsluhy." Nothing in AD-1/AD-2/AD-3 says what happens when a retry occurs after the backend *did* commit the row but the ACK was lost (classic at-least-once problem over a flaky boat Wi-Fi link — the exact environment this project targets). A firmware dev implements "retry until 2xx," which is correct per FR-3. A backend dev, with no uniqueness constraint mandated anywhere in the spine, implements `readings` with an autoincrement PK and no unique index on `captured_at` (or device+captured_at) — because nothing told them one was needed. Both are individually correct against the spine. Together: every lost ACK during a reconnect burst produces a duplicate row, and FR-8's history graph (and SM-4, which validates it) silently gets phantom spikes with no architectural mechanism to prevent or detect it.

**Gap:** AD-1/AD-2/AD-4 govern *that* there is one path and *what units/precision* it carries, but not the wire envelope (single-object vs. batch) or delivery semantics (at-least-once, dedup key). This is the single highest-severity hole because it sits directly on the safety-critical path (FR-4 backfill exists specifically so the rozhodčí's history read after the fact isn't corrupted) and is the boundary most likely to actually be built by two different sessions/times (firmware in C++/PlatformIO vs. backend in Node, days apart).

**Proposed tightening (new AD-8):** Pin the payload envelope explicitly — "one `POST /readings` call carries exactly one reading object; backfill after reconnect is N sequential/sequenced calls, not a batch array" (or the reverse, but *choose one*) — and add an idempotency rule, e.g. "the reading's `capturedAt` is the natural key; the backend upserts (ignore-on-conflict) rather than blind-inserts on `captured_at`," which is cheap to state now and expensive to retrofit after firmware and backend already disagree.

---

## Finding 2 (High) — WebSocket message shape and "who tells the dashboard to re-sync" are unspecified under AD-6

**Units:**
- Unit A — `backend/src/serve` (FR-7 live value, FR-11 staleness, AD-6 WS broadcast)
- Unit B — dashboard live-value + staleness-indicator feature (FR-7, FR-11), living in `backend/src/static`

**What the spine pins:** AD-6 fixes that WS is best-effort and REST (`/readings/latest`, `/readings/history`) is the source of truth, and that the dashboard reconciles via REST "při reconnectu nebo podezření na zastaralost." That's a reconciliation *trigger*, not a message *contract*.

**How two AD-compliant builds diverge:**

- FR-11 requires the dashboard to compute and show data age ("před X s/min") and to visually flag staleness past a threshold. That computation needs a timestamp. A backend dev building `serve/`'s WS broadcaster, reading AD-5 ("směr jako oktant") and AD-4 (finished values only), reasonably ships a minimal WS payload — `{windSpeedMs, windDirOctant}` — because that's literally the data AD-4/AD-5 say the system deals in, and timestamps aren't mentioned in either AD. A dashboard dev, reading FR-11's requirement to show "time since last data," reasonably assumes the WS push carries `capturedAt` (or that receiving *any* WS message means "now," which is wrong — it conflates receive-time with capture-time, exactly the distinction AD-2 exists to prevent, just one layer up the stack where no AD reaches). Neither implementer is wrong per the spine; the two builds produce a dashboard that either can't render "age" from live pushes at all (silently falls back to polling, defeating the point of AD-6) or renders a false "0s old" on every WS tick regardless of actual `captured_at`.

- Backfill silently breaks history-graph freshness. AD-6's reconciliation trigger list is "reconnect nebo podezření na zastaralost" — it does not cover "a backfilled batch of older readings just landed in the DB from firmware reconnecting." A backend dev has no obligation, per any AD, to push anything when `ingest/` inserts backfilled rows (they aren't "the latest" value, so they don't obviously belong on the live WS channel). A dashboard dev has no way to know a mid-session backfill happened unless it independently polls `/readings/history` on some timer of its own invention. FR-8 (history graph) can end up with a gap that self-heals only on the *next* unrelated reconnect/staleness event, or never during a session with a stable connection whose only outage was a brief early-morning one before the dashboard was even opened.

**Gap:** AD-6 specifies *when* to fall back to REST, not *what* WS carries or *what backend-side events* (besides "new live reading") should be signaled at all.

**Proposed tightening:** Extend AD-6 (or add AD-9) to state the WS message is the *same serialized shape* as a `/readings/latest` row (i.e., WS and REST share one DTO, not two independently-designed schemas), and that a backfill insert triggers the same broadcast event type dashboard already listens for (or an explicit `history-changed` signal), so "REST is truth" doesn't quietly become "REST is truth only if the user happens to reconnect after a backfill."

---

## Finding 3 (Medium-High) — RSSI's home is ambiguous: is it a `readings` column or a separate log?

**Units:**
- Unit A — `backend/src/ingest` + `store/` (FR-6: attach and persist RSSI)
- Unit B — `backend/src/serve` + dashboard (FR-6: make RSSI "dohledatelné po akci")

**What the spine pins:** The Naming convention row lists `rssi_dbm` as a `readings`-table-style snake_case column, which *suggests* RSSI lives on the same row as every other reading. AD-1/AD-4 say nothing about it directly (RSSI isn't a "corrected" sensor value, so AD-4's raw-vs-finished distinction doesn't obviously apply).

**How two AD-compliant builds diverge:** FR-6 itself hedges: "Hodnoty RSSI jsou po akci dohledatelné (na dashboardu, **nebo alespoň** v exportovatelném logu)" — the PRD explicitly leaves open whether RSSI is a first-class dashboard-visible field or something recovered only by someone with direct SQLite/file access after the event. A `store/`+`ingest/` builder, taking the Naming convention table as the schema, adds `rssi_dbm` to the `readings` row and considers FR-6 done at persistence time — "it's in the SQLite file, that's the exportable log." A `serve/`+dashboard builder, scoped only to FR-7/FR-8/FR-11/AD-6/AD-7 per the Capability Map (FR-6 isn't even listed against `serve/` in that table — it's listed only under 4.2 against `transmit`/`ingest`), never adds RSSI to `/readings/history`'s response or renders it anywhere, because nothing assigns that responsibility to them. Both are individually defensible against the spine. Result: SM-7 ("Po akci je z RSSI logu zřejmé, jaký dosah Wi-Fi z břehu byl reálně dosažitelný") is only satisfiable by someone SSHing into the server and running raw SQL — which may be acceptable for this hobby project, but it's an accidental outcome of a boundary gap, not a decision anyone made.

**Gap:** The Capability → Architecture Map assigns FR-6 only to `transmit`/`ingest`, never to `serve`, so there is no owner for RSSI's *read side* at all — the spine has a write-only entity.

**Proposed tightening:** Either explicitly scope FR-6 as "persistence only, no dashboard/API surface, evaluated via direct file access post-event" (cheapest, matches hobby-project pragmatism) or add RSSI to the Capability Map row for 4.3/`serve` and state whether it rides along in `/readings/history` or gets its own endpoint. Either is fine — the point is the spine currently implies both simultaneously.

---

## Finding 4 (Medium) — `/readings/history` query and response contract, and where unit conversion is allowed to happen

**Units:**
- Unit A — `backend/src/serve` (FR-8 history endpoint)
- Unit B — dashboard history graph (FR-8) + unit toggle (FR-10)

**What the spine pins:** Consistency Conventions state storage is always m/s and "uzly ... jsou čistě zobrazovací převod v Dashboardu, nikdy uložená hodnota" — this constrains *storage*, not the *wire format of REST responses*. AD-5 pins direction as octant int on both storage and transport. AD-6 names the two endpoints (`/readings/latest`, `/readings/history`) but not their query parameters, pagination, or response envelope.

**How two AD-compliant builds diverge:** FR-8 requires "minimálně data z aktuálního závodního dne" — a `serve/` dev could reasonably add a convenience `?unit=knots` query param to `/readings/history` (still stores m/s, so AD/convention is technically honored — only *serving*, not *storing*, changed), or could default to returning only the last N=500 rows without documenting that as a page size. A dashboard dev, per FR-10's framing that unit conversion is a display-layer concern, reasonably assumes the API always returns raw m/s and always returns the full day's data, and writes client-side conversion + a chart that silently truncates or mis-scales once the backend's undocumented row cap or an added server-side unit param collides with that assumption. Because "uzly jsou čistě zobrazovací převod" is phrased as a storage rule, not an API rule, a server-side unit param doesn't literally break any AD even though it defeats the *spirit* of FR-10.

**Gap:** No AD states that the REST contract (query params, response envelope, and unit) is itself frozen the same way storage units are frozen.

**Proposed tightening:** Extend the Consistency Conventions row (or add a short AD) to state explicitly: `/readings/history` and `/readings/latest` always return `windSpeedMs` in m/s with no server-side unit parameter, and accept an explicit, spine-documented time-range parameter (even a placeholder like "defaults to current calendar day, `?from=`/`?to=` ISO-8601 optional" is enough to remove the ambiguity).

---

## Finding 5 (Low-Medium) — the `readings` schema is illustrated, not pinned

**Units:**
- Unit A — `backend/src/store` (schema owner per Capability Map)
- Unit B — `backend/src/ingest` (writer) / `backend/src/serve` (reader)

**Observation:** The Naming convention row lists four example columns (`captured_at`, `wind_speed_ms`, `wind_dir_octant`, `rssi_dbm`) as illustrations of the snake_case-vs-camelCase rule, not as an exhaustive, typed schema. There's no stated primary key strategy, no stated nullability per column (can `rssi_dbm` be `NULL` for a reading captured before Wi-Fi association completes — a real startup-sequence case, not a hypothetical?), and no stated integer/real types. `store/` is the single named owner of the schema file, which mostly closes off this hole structurally (AD-3's "one process, one storage" plus the folder map means there's exactly one place `CREATE TABLE` can live) — but because the schema itself isn't in the spine, an `ingest/` dev and a `serve/` dev built against *different mental models* of it (e.g., one assumes `rssi_dbm INTEGER NOT NULL`, the other assumes it's nullable for early-boot readings) will only discover the mismatch as a runtime 500 or a silently-dropped row, not as a spine violation either side could have caught by re-reading the document.

**Gap:** This is really Finding 1's nullability sub-case generalized — flagged separately because it also affects `store/`↔`serve/`, not just `store/`↔`ingest/`.

**Proposed tightening:** Fold into AD-8 (Finding 1): pin the full column list with types and nullability, or add a companion `schema.sql`-shaped snippet to the spine's Structural Seed.

---

## Summary Table

| # | Boundary | Units in conflict | Severity | New/tightened AD |
| --- | --- | --- | --- | --- |
| 1 | Stanice → Backend | `firmware/transmit` vs `backend/ingest` | Critical | AD-8: pin POST envelope (single object, not batch) + idempotency key |
| 2 | Backend → Dashboard | `backend/serve` (WS) vs dashboard live/staleness | High | Extend AD-6 / AD-9: WS shares DTO with REST latest; backfill triggers re-sync signal |
| 3 | Backend internal | `ingest+store` vs `serve`+dashboard | Medium-High | Assign FR-6 read-side owner (or explicitly scope RSSI as file-access-only) |
| 4 | Backend → Dashboard | `backend/serve` (history) vs dashboard history/units | Medium | Freeze `/readings/history` query/response contract + forbid server-side unit param |
| 5 | Backend internal | `store` vs `ingest`/`serve` | Low-Medium | Fold into AD-8: pin full `readings` schema incl. nullability |

All five holes share one root cause: the spine is airtight on **topology** (who may write, who may read, which layer corrects, which layer renders) but silent on **wire-level shape** (payload envelope, message DTOs, query contracts, exact schema). For a solo-developer project this is survivable if the same person builds both sides of each boundary in one sitting with the schema fresh in their head — but the spine as written would not catch it if that assumption breaks (time pressure, a different session days apart, or the addendum's mention that Guru may help with parts of the build).
