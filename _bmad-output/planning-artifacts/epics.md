---
stepsCompleted: [step-01-validate-prerequisites, step-02-design-epics, step-03-create-stories, step-04-final-validation]
inputDocuments:
  - _bmad-output/planning-artifacts/prds/prd-gustik-2026-08-01/prd.md
  - _bmad-output/planning-artifacts/prds/prd-gustik-2026-08-01/addendum.md
  - _bmad-output/planning-artifacts/architecture/architecture-gustik-2026-08-01/ARCHITECTURE-SPINE.md
---

# gustik - Epic Breakdown

## Overview

Tento dokument rozkládá PRD (`prd-gustik-2026-08-01`) a Architecture Spine (`architecture-gustik-2026-08-01`) do epik a stories připravených pro implementaci. Žádný UX design spec neexistuje (dashboard je jedna obrazovka, žádné role) — sekce UX Design Requirements je prázdná.

**Rámec, ve kterém byl breakdown udělaný:** jeden vývojář (Mlok), termín 17. 8. 2026 (v době psaní tohoto dokumentu 16 dní), priorita z PRD kap. 6.2: **funkční živé čtení > historický graf > vyladěná mechanika**. Epiky jsou seřazené přesně v tomto pořadí; v rámci každé epiky jsou stories seřazené tak, aby žádná nezávisela na budoucí story.

## Requirements Inventory

### Functional Requirements

```
FR-1: Stanice měří okamžitou rychlost větru z anemometru (pulsy z reed spínače → m/s, vzorkovací interval ~2–5 s).
FR-2: Stanice měří relativní směr větru z korouhve a koriguje ho o natočení lodi (magnetometr) na 8 kardinálních směrů (oktantů).
FR-3: Stanice se připojuje k předkonfigurované Wi-Fi (břeh/mobilní hotspot) a periodicky odesílá data na Backend; při ztrátě signálu se sama pokouší znovu připojit bez restartu/zásahu obsluhy.
FR-4: Při výpadku spojení Stanice ukládá data lokálně (buffer ≥4 h) a po obnovení spojení je doplní (backfill) do historie ve správném časovém pořadí; best-effort, ne garance.
FR-5: Stanice signalizuje ztrátu spojení přímo na jednotce (LED) do 10 s od zjištění výpadku, signalizace zhasne po obnovení.
FR-6: Stanice zaznamenává RSSI Wi-Fi signálu ke každému měření; hodnoty jsou po akci dohledatelné pro vyhodnocení dosahu.
FR-7: Dashboard zobrazuje aktuální rychlost a směr větru, čitelné na první pohled, aktualizované bez ručního refreshe, responzivní (mobil i PC).
FR-8: Dashboard zobrazuje historický graf rychlosti a směru větru minimálně za aktuální závodní den.
FR-9: Dashboard je dostupný přes sdílený odkaz bez přihlašování — žádné role ani oprávnění.
FR-10: Dashboard umožňuje přepnout jednotku rychlosti větru mezi m/s a uzly (jen zobrazovací převod, uložení zůstává v m/s).
FR-11: Dashboard viditelně ukazuje stáří posledního přijatého záznamu a vizuálně odliší data starší než 2 minuty jako potenciálně neplatná.
FR-12: Stanice běží nepřetržitě ≥8 h na jedno nabití USB-C powerbanky (typický závodní den 9:00–17:00).
FR-13: Elektronika a senzory jsou chráněné před stříkající vodou a deštěm (ne ponoření) a bezpečně upevněné na kotvící, stáčející se lodi.
FR-14: Stanice má napevno nakonfigurované Wi-Fi přihlašovací údaje (1–2 sítě) a připojuje se sama bez jakéhokoli zásahu obsluhy v terénu; rekonfigurace je servisní úkon (Mlok, mimo den akce) přes soubor, ne přeflashování.
FR-15: Existuje psaný instalační/provozní návod dostupný online, srozumitelný pro rozhodčího bez technického zázemí, pokrývající zapnutí, LED indikaci, ověření čerstvých dat na dashboardu a postup při selhání.
```

### NonFunctional Requirements

```
NFR-1: Výdrž napájení ≥8 h nepřetržitého provozu na jedno nabití USB-C powerbanky (odvozeno z FR-12/SM-2).
NFR-2: Reakční doba LED signalizace výpadku spojení ≤10 s od detekce (FR-5).
NFR-3: Práh "zastaralých dat" na dashboardu = 2 minuty bez nového záznamu; nezávislý na 10s prahu LED (FR-11).
NFR-4: Kapacita lokálního bufferu na Stanici ≥4 h dat při obvyklé vzorkovací frekvenci, než začne přepisovat nejstarší (FR-4).
NFR-5: Mechanická odolnost pouzdra vůči stříkající vodě a dešti (ne ponoření) po dobu typického provozu na regatě (FR-13).
NFR-6: Rozlišení směru větru je limitované na 8 kardinálních směrů (dané hardwarem korouhve) — vyšší přesnost směru není v v1 cíl (FR-2).
NFR-7: Dashboard musí být použitelný jak na mobilním telefonu, tak na PC (responzivní layout, FR-7).
NFR-8: Zápisový endpoint (`POST /readings`) vyžaduje autentizaci sdíleným tokenem; čtecí přístup k Dashboardu zůstává vědomě bez přihlašování (FR-9 vs. AD-10) — nejde o obecný autentizační systém.
NFR-9: Gustik je podpora rozhodování, ne automatizovaný bezpečnostní systém — v1 vědomě nemá žádné automatické alerty při překročení prahu větru; žádná story nesmí zavést automatizované rozhodování/alerting nad rámec zobrazení čísla.
NFR-10: Přesnost měření není certifikovaná ani validovaná proti referenčnímu přístroji — `[NOTE FOR PM přenesené z PRD]` zvážit v UX kopii dashboardu poznámku o orientační povaze dat (nezávazné, nemá vlastní FR).
```

### Additional Requirements

```
- Paradigma: jednosměrná layered pipeline Sense → Correct → Buffer/Transmit (firmware) → Ingest → Store → Serve (backend) → Render (dashboard); žádná fáze nezná vnitřní implementaci sousední fáze (AD-7).
- AD-1: Stanice zapisuje měření výhradně přes jediný HTTP POST endpoint (`POST /readings`) — žádná druhá cesta zápisu.
- AD-2: Řazení podle `captured_at` (čas měření na Stanici), ne `received_at`; Stanice synchronizuje hodiny přes SNTP při bootu a po každém reconnectu; bez úspěšné synchronizace `captured_at` spadá zpět na odhad z `received_at` (`clock_synced` flag).
- AD-3: Backend běží jako jeden proces (Fastify) nad jedním SQLite souborem (`better-sqlite3`) — žádná fronta, cache ani druhé úložiště v v1.
- AD-4: Yaw-korekce a převod na SI jednotky probíhají výhradně na Stanici (firmware) — Backend nikdy nepřijímá surové čtení senzoru.
- AD-5: Směr větru se ukládá i přenáší jako celé číslo 0–7 (oktant), nikdy jako stupně.
- AD-6: WebSocket kanál pro živé hodnoty je best-effort optimalizace; REST `/readings/latest` a `/readings/history` jsou jediný zdroj pravdy, na který se Dashboard vždy dorovná při reconnectu/podezření na zastaralost.
- AD-7: Závislost jde vždy jen Stanice → Backend → Dashboard; Backend neobsahuje žádnou hardwarově specifickou znalost; Dashboard mluví výhradně s Backend API.
- AD-8: `POST /readings` vždy přijímá JSON pole 1..N záznamů (živé odeslání = pole délky 1, backfill = pole N záznamů). Každý záznam nese firmwarem vygenerované `client_id` s `UNIQUE` constraintem na Backendu — opakované odeslání je no-op, nikdy duplicitní řádek.
- AD-9: Každá WS zpráva nese identický tvar záznamu jako REST (`capturedAt`, `windSpeedMs`, `windDirOctant`, `rssiDbm`). Vložení záznamu s `capturedAt` starším než poslední viděný spustí holou WS událost `history-changed`, která řekne Dashboardu znovu načíst `/readings/history` (ne inkrementální patch).
- AD-10: `POST /readings` vyžaduje statický bearer token (Stanice: konfigurační soubor, Backend: env proměnná) — hlídá jen tento zápisový endpoint, čtení Dashboardu zůstává bez přihlašování.
- Konvence pojmenování: SQLite sloupce `snake_case`, HTTP JSON klíče `camelCase`; převod výhradně ve vrstvě DB přístupu (`backend/src/store`).
- Časová razítka jako ISO-8601 UTC řetězec na drátě; rychlost větru vždy v m/s na drátě/v úložišti — uzly jsou čistě klientský zobrazovací převod, `/readings/history` nikdy nepřijímá parametr jednotky.
- Wi-Fi přihlašovací údaje a ingest token žijí v samostatném konfiguračním souboru na flash Stanice, nikdy zakompilované ve firmware zdrojáku (podporuje FR-14 servisní rekonfiguraci bez přeflashování).
- Firmware se při chybě odeslání nikdy neblokuje ani nezastavuje vzorkování — vždy spadne do lokálního bufferu (FR-4) a pokračuje ve čtení senzorů.
- Pinned schéma tabulky `readings`: `id`, `client_id` (UNIQUE), `captured_at`, `received_at`, `clock_synced`, `wind_speed_ms`, `wind_dir_octant`, `rssi_dbm` (nullable — NULL před prvním úspěšným Wi-Fi scanem), `backfilled` (bool).
- Stack (pinned v architektuře, ne k rozhodování na úrovni story): Node.js 24 + Fastify 5.11.x + better-sqlite3 13.0.x + `@fastify/websocket` + `@fastify/static`; vanilla JS + Chart.js 4.5.x bez build kroku; firmware C++/Arduino framework (ESP32 core 2.x) přes PlatformIO; magnetometr knihovna kompatibilní s HMC5883L i QMC5883L (např. `DFRobot_QMC5883`) kvůli riziku discontinued pravého HMC5883L čipu.
- Deployment: Docker/Compose za existující Caddy reverzní proxy (HTTPS), restart policy `unless-stopped`, SQLite na named volume, `GET /health` endpoint vracející 200, jakmile jde otevřít SQLite soubor.
- Žádný starter template není architekturou předepsaný — Epic 1 Story 1.3 zakládá backend projekt (Fastify + better-sqlite3 + Docker skeleton) od nuly jako součást prvního backend endpointu, ne jako samostatný "setup" krok bez uživatelské hodnoty.
```

### UX Design Requirements

```
N/A — pro Gustik v1 nebyl vytvořen samostatný UX design spec (`bmad-ux`). Dashboard je jedna veřejná obrazovka bez rolí; UX-relevantní požadavky (responzivita, čitelnost na první pohled, indikace stáří dat, jednotkový přepínač) jsou pokryté přímo funkčními požadavky FR-7, FR-9, FR-10, FR-11 a promítnuté do story acceptance criteria v Epic 1 a Epic 3.
```

### FR Coverage Map

```
FR-1:  Epic 1 (Story 1.1) — měření rychlosti větru
FR-2:  Epic 1 (Story 1.2) — měření směru s yaw-korekcí
FR-3:  Epic 1 (Story 1.3, 1.4) — ingest endpoint + odesílání s auto-reconnectem
FR-4:  Epic 2 (Story 2.1, 2.2) — lokální buffer + backfill
FR-5:  Epic 2 (Story 2.4) — LED signalizace výpadku
FR-6:  Epic 2 (Story 2.5) — RSSI logování
FR-7:  Epic 1 (Story 1.5, 1.6) — live hodnota (backend serve + dashboard)
FR-8:  Epic 3 (Story 3.1, 3.2) — historický graf
FR-9:  Epic 1 (Story 1.6) — veřejný přístup bez přihlašování
FR-10: Epic 1 (Story 1.6) — přepínač jednotek
FR-11: Epic 1 (Story 1.6) — indikace stáří dat
FR-12: Epic 5 (Story 5.1) — výdrž na powerbance
FR-13: Epic 5 (Story 5.2) — voděodolné pouzdro a upevnění
FR-14: Epic 4 (Story 4.1) — bezobslužné připojení k Wi-Fi
FR-15: Epic 4 (Story 4.2) — psaný návod
```

## Epic List

### Epic 1: Základní živý přenos větru (walking skeleton)
Rozhodčí i vedoucí vidí na dashboardu aktuální rychlost a směr větru, živě, veřejně bez přihlašování, z mobilu i PC, s viditelnou informací o stáří dat. Toto je kompletní, samostatně funkční systém za předpokladu spolehlivého spojení (FR-3 už obsahuje auto-reconnect) — realizuje jádro PRD priority č. 1 "funkční živé čtení" a UJ-1/UJ-2/SM-3.
**FRs covered:** FR-1, FR-2, FR-3, FR-7, FR-9, FR-10, FR-11

### Epic 2: Odolnost spojení a diagnostika v terénu
Nad fungující živý přenos (Epic 1) epika přidává odolnost proti výpadkům spojení na vodě: lokální buffer s doplněním historie po výpadku, fyzickou LED signalizaci na Stanici a logování síly Wi-Fi signálu pro vyhodnocení dosahu po akci. Realizuje UJ-1 edge case a SM-7.
**FRs covered:** FR-4, FR-5, FR-6

### Epic 3: Historický graf
Rozhodčí si po skončení závodění (nebo mezi rozjezdy) prohlédne, jak se vítr měnil v průběhu dne — realizuje PRD prioritu č. 2 "historický graf", UJ-3 a SM-4. Staví na úložišti z Epic 1 a na backfill/WS eventu z Epic 2 (nezávisí na nich funkčně navzájem obráceně).
**FRs covered:** FR-8

### Epic 4: Bezobslužné zprovoznění v terénu
Rozhodčí bez technického zázemí dokáže sám ráno zprovoznit Stanici podle psaného návodu, bez zásahu Mloka — Stanice se připojí k předkonfigurované Wi-Fi bez jakékoli terénní konfigurace. Realizuje UJ-4 a SM-6.
**FRs covered:** FR-14, FR-15

### Epic 5: Výdrž napájení a voděodolná mechanická instalace
Stanice vydrží nabitá a chráněná před vodou celý závodní den bez zásahu — bezpečnostní/spolehlivostní předpoklad, na kterém stojí všechny ostatní epiky (bez napájení a mechanické ochrany neběží nic). Realizuje PRD prioritu č. 3 "vyladěná mechanika", SM-2 a SM-5. Zařazena poslední v pořadí implementace podle PRD priority, ale prakticky souběžná s hardwarovou stavbou po celou dobu projektu.
**FRs covered:** FR-12, FR-13

---

## Epic 1: Základní živý přenos větru (walking skeleton)

Rozhodčí i vedoucí vidí na dashboardu aktuální rychlost a směr větru, živě, veřejně bez přihlašování, z mobilu i PC, s viditelnou informací o stáří dat.

**Relevantní NFR/dodatečné požadavky:** NFR-6 (8 oktantů), NFR-7 (responzivita), NFR-8 (auth jen na zápisu), NFR-9 (žádné automatické alerty), NFR-10 (orientační povaha dat — volitelná kopie), AD-1, AD-4, AD-5, AD-6, AD-7, AD-8 (obálka pole 1..N od začátku, i když Epic 1 vždy posílá pole délky 1), AD-10, pinned schéma `readings`, konvence snake_case/camelCase.

### Story 1.1: Měření rychlosti větru z anemometru

As a stavitel (Mlok),
I want aby Stanice počítala pulsy z reed spínače anemometru a převáděla je na rychlost větru v m/s v pravidelném intervalu,
So that mám první surová, ale použitelná data o síle větru k dalšímu zpracování a odeslání.

**Acceptance Criteria:**

**Given** anemometr (reed spínač) je zapojený na ESP32 GPIO pin s interrupt-driven čítáním pulsů
**When** Stanice běží a vítr otáčí anemometrem
**Then** firmware spočítá pulsy za definovaný vzorkovací interval (~2–5 s) a převede je na rychlost větru v m/s
**And** hodnota je uložená interně v proměnné/struktuře v m/s (FR-1) — žádná jiná jednotka se v tomto kroku nepočítá

**Given** anemometr stojí (bezvětří)
**When** proběhne vzorkovací cyklus
**Then** vypočtená rychlost je 0 m/s, ne chyba ani NaN

### Story 1.2: Měření směru větru s yaw-korekcí na 8 oktantů

As a stavitel (Mlok),
I want aby Stanice četla surový směr z korouhve a magnetometru a korigovala ho o aktuální natočení kotvící lodi na jeden z 8 kardinálních směrů,
So that zobrazený směr větru je vztažený k severu, ne k přídi otáčející se lodi.

**Acceptance Criteria:**

**Given** korouhev (odporový dělič) a magnetometr (I2C, HMC5883L/QMC5883L-kompatibilní knihovna) jsou zapojené na ESP32
**When** Stanice provede vzorkovací cyklus
**Then** firmware přečte surový směr korouhve (jeden z 8 nativních směrů) a aktuální natočení lodi z magnetometru
**And** korigovaný výsledný směr je surový směr + natočení, zaokrouhlený na nejbližší z 8 oktantů (celé číslo 0–7, AD-5) — nikdy stupně

**Given** kotvící loď se stáčí (natočení magnetometru se mění)
**When** skutečný směr větru vůči severu zůstává stejný
**Then** korigovaný oktant se nemění jen kvůli otočení lodi (potvrzuje účel korekce z FR-2)

**Given** magnetometr ještě neprošel hard-iron/soft-iron kalibrací pro konkrétní loď
**When** firmware čte magnetometr
**Then** kód počítá s tím, že kalibrační konstanty jsou konfigurovatelné (ne hardcoded magická čísla bez pojmenování) — kalibrace samotná je servisní/mechanický úkon mimo tuto story

### Story 1.3: Ingest endpoint — autentizovaný, idempotentní zápis měření

As a stavitel (Mlok),
I want backend endpoint `POST /readings`, který přijme pole 1..N naměřených záznamů, ověří sdílený token a zapíše je do SQLite bez duplicit,
So that Stanice má kam bezpečně odeslat data a historie nikdy neobsahuje falešné nebo duplicitní řádky.

**Acceptance Criteria:**

**Given** nový Fastify backend projekt (Node.js 24, `better-sqlite3` 13.0.x, Docker skeleton s `node:24-bookworm-slim`) zatím neexistuje
**When** tato story začíná
**Then** je založená minimální struktura `backend/src/ingest`, `backend/src/store` a SQLite schéma tabulky `readings` přesně dle pinned schématu (`id`, `client_id` UNIQUE, `captured_at`, `received_at`, `clock_synced`, `wind_speed_ms`, `wind_dir_octant`, `rssi_dbm` nullable, `backfilled`)

**Given** backend běží a je nakonfigurovaný sdílený bearer token (env proměnná)
**When** přijde `POST /readings` s platným tokenem a JSON polem obsahujícím 1 záznam s `client_id`, `capturedAt`, `windSpeedMs`, `windDirOctant`
**Then** endpoint zapíše záznam do SQLite (`received_at` = teď, `backfilled` = false), převede camelCase → snake_case ve `store` vrstvě, a vrátí 2xx

**Given** backend obdrží `POST /readings` bez tokenu nebo s neplatným tokenem
**When** request dorazí
**Then** endpoint vrátí 401 a nic nezapíše (AD-10)

**Given** backend už jednou přijal záznam s daným `client_id`
**When** přijde druhý `POST /readings` se stejným `client_id` (opakované odeslání po ztraceném ACK)
**Then** zápis je no-op (žádný duplicitní řádek), endpoint vrátí úspěšnou odpověď (AD-8 idempotence)

**Given** `GET /health` je zavolán
**When** SQLite soubor jde otevřít
**Then** endpoint vrátí 200

### Story 1.4: Odesílání živých dat přes Wi-Fi s automatickým reconnectem

As a rozhodčí na lodi (Tomáš, UJ-1),
I want aby Stanice sama odesílala naměřená data na Backend přes Wi-Fi a při výpadku signálu se sama znovu připojila,
So that nemusím se Stanicí jakkoliv manipulovat, aby fungovala.

**Acceptance Criteria:**

**Given** Stanice má naměřená data z 1.1/1.2 a zná adresu Backendu + ingest token (viz Epic 4 pro zdroj konfigurace; v této story stačí staticky zadaná hodnota)
**When** Stanice je připojená k Wi-Fi
**Then** po každém vzorkovacím cyklu odešle `POST /readings` s polem o délce 1 obsahujícím aktuální měření

**Given** Stanice ztratí Wi-Fi signál
**When** proběhne další pokus o odeslání
**Then** odeslání selže tiše (bez pádu/resetu firmware) a Stanice se pokusí o opětovné připojení k Wi-Fi bez restartu nebo zásahu obsluhy (FR-3)

**Given** Wi-Fi signál se obnoví
**When** proběhne další reconnect pokus
**Then** Stanice se úspěšně znovu připojí a pokračuje v pravidelném odesílání živých dat

### Story 1.5: Backend serve — aktuální hodnota přes REST a WebSocket

As a stavitel (Mlok),
I want backend endpointy `GET /readings/latest` (REST) a WebSocket kanál vysílající každý nově přijatý záznam ve stejném tvaru,
So that Dashboard má odkud číst aktuální hodnotu jak při načtení stránky, tak živě bez ručního refreshe.

**Acceptance Criteria:**

**Given** v SQLite existuje alespoň jeden záznam
**When** přijde `GET /readings/latest`
**Then** endpoint vrátí nejnovější záznam podle `captured_at` (ne podle pořadí vložení, AD-2) ve tvaru `{capturedAt, windSpeedMs, windDirOctant, rssiDbm}` (camelCase, AD-9 tvar)

**Given** WebSocket klient je připojený (`@fastify/websocket`)
**When** ingest endpoint (Story 1.3) úspěšně zapíše nový živý záznam
**Then** backend vyšle na WS kanál zprávu identického tvaru jako REST záznam (AD-9) — žádný částečný/jinak tvarovaný push

**Given** v SQLite zatím není žádný záznam
**When** přijde `GET /readings/latest`
**Then** endpoint vrátí jasně rozlišitelnou "žádná data" odpověď (ne 500, ne fingovaná nulová hodnota)

### Story 1.6: Dashboard — veřejné živé zobrazení s jednotkami a indikací stáří

As a rozhodčí nebo vedoucí oddílu (UJ-1, UJ-2),
I want otevřít veřejný odkaz na Dashboard z mobilu i PC bez přihlašování a vidět aktuální rychlost a směr větru, s možností přepnout jednotku a s jasnou indikací, jak stará data jsou,
So that se mohu rozhodnout o bezpečnosti/instruktáži posádky na základě čísla, kterému věřím, nebo poznat, že je zastaralé.

**Acceptance Criteria:**

**Given** uživatel otevře URL dashboardu na mobilu nebo PC bez jakéhokoli přihlašování (FR-9)
**When** stránka se načte
**Then** zobrazí se aktuální rychlost a směr větru staženy z `GET /readings/latest`, layout je čitelný a responzivní na obou typech zařízení (FR-7, NFR-7)

**Given** dashboard je otevřený a přijde nová WS zpráva (Story 1.5)
**When** zpráva dorazí
**Then** zobrazená hodnota se aktualizuje bez nutnosti ručního obnovení stránky (FR-7); WS je jen optimalizace — při reconnectu WS klienta se dashboard vždy dorovná přes `GET /readings/latest` (AD-6)

**Given** dashboard zobrazuje aktuální rychlost větru
**When** uživatel přepne jednotku m/s ↔ uzly
**Then** zobrazená hodnota se přepočítá čistě na klientovi (FR-10); žádný request na backend nenese parametr jednotky

**Given** poslední přijatý záznam je starší než 2 minuty (NFR-3)
**When** dashboard vykresluje aktuální hodnotu
**Then** hodnota je vizuálně odlišená jako potenciálně neplatná (např. přeškrtnutí/změna barvy) a je vidět čas posledního záznamu ("před X s/min", FR-11)

**Given** směr větru je oktant 0–7 z backendu
**When** dashboard ho vykresluje
**Then** zobrazí se jako jeden z 8 kardinálních směrů (např. šipka/text), nikdy jako vymyšlený přesný stupeň (NFR-6, AD-5)

---

## Epic 2: Odolnost spojení a diagnostika v terénu

Nad živý přenos z Epic 1 přidává odolnost proti výpadkům spojení: lokální buffer, doplnění historie po výpadku, LED signalizaci a RSSI log.

**Relevantní NFR/dodatečné požadavky:** NFR-2 (LED ≤10s), NFR-4 (buffer ≥4h), AD-2 (řazení podle `captured_at`, `clock_synced`), AD-8 (pole N záznamů pro backfill), AD-9 (backend strana `history-changed` eventu).

### Story 2.1: Lokální buffer při výpadku spojení

As a rozhodčí na lodi (UJ-1, edge case),
I want aby Stanice při výpadku spojení nepřestala měřit a ukládala si naměřená data lokálně,
So that ve chvíli výpadku nevznikne v historii díra a Stanice se nezasekne kvůli síťové chybě.

**Acceptance Criteria:**

**Given** Stanice nemůže úspěšně odeslat `POST /readings` (Story 1.4 selhalo)
**When** proběhne další vzorkovací cyklus
**Then** naměřený záznam se uloží do lokálního bufferu na flash paměti místo zahození; vzorkování senzorů pokračuje beze změny intervalu (firmware se nikdy neblokuje na chybě odeslání)

**Given** lokální buffer drží data po dobu odpovídající alespoň 4 hodinám při obvyklé vzorkovací frekvenci (NFR-4)
**When** buffer dosáhne kapacity a spojení je stále nedostupné
**Then** Stanice začne přepisovat nejstarší záznamy (best-effort, FR-4) — nespadne a nepřestane vzorkovat

### Story 2.2: Doplnění bufferovaných dat po obnovení spojení (backfill)

As a stavitel (Mlok) / rozhodčí vyhodnocující den zpětně (UJ-3),
I want aby se po obnovení Wi-Fi spojení bufferovaná data odeslala na Backend ve správném časovém pořadí,
So that historie na dashboardu neobsahuje díru za dobu výpadku.

**Acceptance Criteria:**

**Given** lokální buffer obsahuje N nedoručených záznamů a Wi-Fi/Backend spojení se obnoví
**When** Stanice detekuje obnovené spojení
**Then** odešle bufferovaná data na `POST /readings` jako JSON pole N záznamů (AD-8) seřazené podle `captured_at`, každý se svým `client_id`

**Given** backfill odeslání selže uprostřed (např. spojení znovu vypadne)
**When** Stanice zkusí backfill znovu při dalším obnovení
**Then** už jednou úspěšně přijaté záznamy se díky `client_id` UNIQUE constraintu (Story 1.3) nezapíšou podruhé — žádné duplicity v historii

**Given** Stanice od bootu nikdy neprovedla úspěšnou SNTP synchronizaci hodin
**When** ukládá záznam do bufferu i při backfillu
**Then** `captured_at` je označen jako `clock_synced = false`, aby Backend/Store mohl podle AD-2 zacházet s časem jako s odhadem, ne jistotou

### Story 2.3: Backend označí backfillovaná data a upozorní na potřebu resynchronizace

As a stavitel (Mlok),
I want aby backend při zápisu záznamu, který přišel mimo živé odeslání (starší `captured_at` než poslední známý), nastavil `backfilled = true` a poslal WS událost `history-changed`,
So that jakýkoli budoucí konzument (dashboard) ví, že si má znovu načíst historii, místo aby tiše zůstal se starým grafem.

**Acceptance Criteria:**

**Given** ingest endpoint (Story 1.3) přijme pole záznamů, jejichž `captured_at` je starší než poslední doposud přijatý záznam
**When** backend je zapisuje do SQLite
**Then** tyto záznamy se uloží s `backfilled = true` (živě odeslaná data mají `backfilled = false`)

**Given** alespoň jeden zapsaný záznam v dávce má `backfilled = true`
**When** zápis dávky doběhne
**Then** backend vyšle na WS kanál holou událost `history-changed` (bez payloadu záznamů, AD-9) — odděleně od běžných per-záznam WS zpráv ze Story 1.5

**Given** v tuto chvíli (před Epic 3) žádný klient na `history-changed` neposlouchá
**When** událost je vyslána
**Then** to nezpůsobí chybu ani pád backendu — je to harmless broadcast, který Epic 3 later využije

### Story 2.4: LED signalizace ztráty spojení

As a rozhodčí fyzicky u Stanice (UJ-1),
I want aby Stanice fyzickou LED signalizovala, když nemůže odeslat data na Backend,
So that i bez pohledu na dashboard poznám, že aktuálně nejdou živá data.

**Acceptance Criteria:**

**Given** Stanice detekuje, že se jí nedaří odeslat/potvrdit data na Backend
**When** uplyne od zjištění výpadku maximálně 10 s (NFR-2)
**Then** LED na jednotce se rozsvítí/změní stav do signalizace výpadku (FR-5)

**Given** LED signalizuje výpadek
**When** se spojení obnoví a další odeslání uspěje
**Then** LED zhasne/vrátí se do normálního stavu

### Story 2.5: Logování RSSI Wi-Fi signálu

As a stavitel (Mlok),
I want aby každý odeslaný i bufferovaný záznam nesl aktuální sílu Wi-Fi signálu (RSSI) v okamžiku měření,
So that po akci mohu vyhodnotit, jaký dosah Wi-Fi z břehu byl reálně dosažitelný (SM-7).

**Acceptance Criteria:**

**Given** Stanice provádí vzorkovací cyklus (Story 1.1/1.2) a je připojená k Wi-Fi
**When** se sestavuje záznam k odeslání/bufferování
**Then** záznam obsahuje `rssiDbm` naměřené v okamžiku měření (standardní ESP32 Wi-Fi API)

**Given** Stanice ještě neprovedla první úspěšný Wi-Fi scan po bootu
**When** se sestavuje záznam
**Then** `rssiDbm` je `NULL` (pinned schéma povoluje nullable) — čtecí strana (Story 1.5/3.1) to musí zvládnout, ne předpokládat vždy číslo

**Given** akce skončila a existuje historie záznamů s RSSI
**When** Mlok chce vyhodnotit dosah
**Then** hodnoty RSSI jsou dohledatelné přes existující `/readings/history` (Epic 3) nebo přímý SQLite dotaz — přesná forma zobrazení na dashboardu není v rozsahu této story (FR-6 nevyžaduje dedikovanou vizualizaci, jen dohledatelnost)

---

## Epic 3: Historický graf

Rozhodčí si prohlédne, jak se vítr měnil v průběhu dne.

**Relevantní NFR/dodatečné požadavky:** AD-6 (`/readings/history` jako zdroj pravdy), AD-9 (frontend strana `history-changed`), Chart.js 4.5.x bez build kroku.

### Story 3.1: Backend endpoint pro historii měření

As a stavitel (Mlok),
I want backend endpoint `GET /readings/history`, který vrátí časovou řadu záznamů za aktuální závodní den,
So that Dashboard má odkud vykreslit graf.

**Acceptance Criteria:**

**Given** v SQLite existují záznamy s různými `captured_at` v rámci aktuálního dne
**When** přijde `GET /readings/history`
**Then** endpoint vrátí pole záznamů seřazené podle `captured_at` vzestupně, minimálně pokrývající aktuální závodní den (FR-8), ve stejném tvaru záznamu jako `/readings/latest` a WS zprávy (camelCase, AD-9)

**Given** endpoint je zavolaný
**When** request nenese žádný parametr jednotky
**Then** odpověď vždy vrací `windSpeedMs` v SI — endpoint nikdy nepřijímá ani neaplikuje jednotkový převod (Consistency Conventions)

**Given** v historii je záznam s `rssiDbm = NULL` (Story 2.5 edge case)
**When** je součástí odpovědi
**Then** endpoint ho vrátí tak, jak je (`null`), bez pádu na serializaci

### Story 3.2: Dashboard — historický graf rychlosti a směru

As a rozhodčí vyhodnocující den (Tomáš, UJ-3),
I want na dashboardu vidět graf vývoje rychlosti a směru větru v čase,
So that mohu vizuálně srovnat podmínky mezi různými částmi dne při plánování dalšího rozpisu/ročníku.

**Acceptance Criteria:**

**Given** uživatel otevře dashboard (Story 1.6) obsahující sekci historie
**When** stránka načte data z `GET /readings/history` (Story 3.1)
**Then** vykreslí se graf (Chart.js) zobrazující rychlost i směr větru v čase, minimálně za aktuální závodní den (FR-8)

**Given** graf je vykreslený v m/s
**When** uživatel přepne jednotku na dashboardu (Story 1.6, FR-10)
**Then** graf rychlosti se přepočítá na uzly čistě na klientovi, stejně jako aktuální hodnota

**Given** uživatel se dívá na graf
**When** vizuálně srovnává období
**Then** dokáže rozeznat období silnějšího/slabšího větru bez jakékoli dedikované funkce pro tagování rozjezdů (mimo rozsah, PRD kap. 5)

### Story 3.3: Resynchronizace grafu po backfillu

As a rozhodčí vyhodnocující den (UJ-3),
I want aby se graf automaticky doplnil o data, která dorazila se zpožděním po výpadku spojení (backfill),
So that historie, kterou vidím, je úplná i po výpadku, aniž bych musel ručně obnovovat stránku.

**Acceptance Criteria:**

**Given** dashboard má otevřený WS kanál (Story 1.5) a backend vyšle `history-changed` (Story 2.3, po backfillu)
**When** dashboard přijme tuto událost
**Then** znovu načte `GET /readings/history` a překreslí graf (celé přenačtení, ne pokus o inkrementální patch, AD-9)

**Given** WS kanál je dočasně odpojený, když backfill proběhne
**When** se WS znovu připojí
**Then** dashboard se při reconnectu WS vždy dorovná přes REST (AD-6) — i bez zachyceného `history-changed` eventu zůstává historie nakonec konzistentní při dalším ručním/periodickém načtení

---

## Epic 4: Bezobslužné zprovoznění v terénu

Rozhodčí bez technického zázemí dokáže sám ráno zprovoznit Stanici podle psaného návodu.

**Relevantní NFR/dodatečné požadavky:** konfigurační soubor odděleně od firmware zdrojáku (Consistency Conventions), AD-10 (token ve stejné konvenci jako Wi-Fi credentials).

### Story 4.1: Bezobslužné připojení k předkonfigurované Wi-Fi

As a rozhodčí zprovozňující Stanici (Petra, UJ-4),
I want aby se Stanice po zapnutí sama připojila k jedné ze dvou předem nastavených Wi-Fi sítí, bez jakékoli mé konfigurace,
So that jediné, co musím udělat, je zapojit Stanici do powerbanky a sledovat LED.

**Acceptance Criteria:**

**Given** Stanice má na flash paměti konfigurační soubor s SSID/heslem pro 1–2 známé sítě a ingest tokenem (odděleně od firmware zdrojáku, ne zakompilované)
**When** Stanice se zapne
**Then** se sama pokusí připojit k dostupné z nakonfigurovaných sítí, bez zásahu obsluhy — žádný notebook, žádné zadávání hesel v terénu (FR-14)

**Given** obě předkonfigurované sítě jsou v dosahu
**When** Stanice se připojuje
**Then** připojí se k jedné z nich podle definovaného chování (např. prioritní pořadí v configu); `[OTEVŘENÁ OTÁZKA přenesená z PRD/architektury]` fyzický přepínač břeh/mobil zůstává mimo rozsah této story — přidá se jen pokud automatický fallback v praxi nestačí (viz Deferred v architektuře)

**Given** Mlok potřebuje před další akcí změnit SSID/heslo
**When** provádí servisní zásah
**Then** stačí upravit konfigurační soubor přes fyzické připojení (sériová linka/USB), bez přeflashování celého firmware

### Story 4.2: Psaný instalační a provozní návod

As a rozhodčí bez technického zázemí (Petra, UJ-4),
I want dostupný psaný návod online, který mi ráno řekne přesně, co mám udělat, abych Stanici zprovoznila,
So that zvládnu ranní zprovoznění sama, bez volání Mlokovi.

**Acceptance Criteria:**

**Given** existuje stránka/dokument publikovaný online (servírovaný přes `backend/src/static`, provázaný s dashboardem)
**When** rozhodčí ho otevře na svém telefonu
**Then** obsahuje alespoň: zapnutí Stanice a powerbanky, co dělat s případným přepínačem břeh/mobil (pokud existuje), jak poznat úspěšné připojení podle LED (Story 2.4/FR-5), jak ověřit na dashboardu čerstvá data (Story 1.6/FR-11), a co dělat, když se něco z toho nepovede (FR-15)

**Given** se použije záložní mobilní hotspot
**When** návod tuto variantu popisuje
**Then** uvádí přesné SSID a heslo, které si má rozhodčí na svém telefonu nastavit — musí se shodovat s tím, co je nahrané ve Stanici (Story 4.1)

**Given** rozhodčí je na místě bez přístupu k vývojářskému prostředí
**When** potřebuje návod znovu otevřít
**Then** je dostupný online (ne jen v repozitáři kódu nebo v hlavě Mloka)

---

## Epic 5: Výdrž napájení a voděodolná mechanická instalace

Stanice vydrží nabitá a chráněná před vodou celý závodní den bez zásahu.

**Relevantní NFR/dodatečné požadavky:** NFR-1 (≥8h), NFR-5 (splash/rain, ne ponoření).

### Story 5.1: Celodenní provoz na USB-C powerbance

As a rozhodčí spoléhající na Stanici celý den (UJ-1, UJ-2),
I want aby Stanice po plném nabití powerbanky běžela nepřetržitě minimálně 8 hodin bez výpadku napájení,
So that nemusím Stanici během závodního dne dobíjet nebo měnit powerbanku.

**Acceptance Criteria:**

**Given** kompletní Stanice (ESP32 + senzory + magnetometr + Wi-Fi rádio) je připojená k plně nabité USB-C powerbance
**When** proběhne test reálného provozu odpovídající typickému závodnímu dni (9:00–17:00, se vzorkováním a odesíláním dle Epic 1/2)
**Then** Stanice běží nepřerušeně minimálně 8 hodin bez výpadku napájení (FR-12, SM-2)

**Given** test odhalí, že spotřeba (pravděpodobně dominantně Wi-Fi vysílání) nedosahuje cíle
**When** Mlok vyhodnocuje výsledek
**Then** je to zaznamenáno jako riziko k řešení (větší powerbanka, úprava vzorkovacího/odesílacího intervalu) před nasazením 17. 8. — mimo rozsah samotné story je redesign firmware spotřeby, pokud test projde

### Story 5.2: Voděodolné pouzdro a upevnění senzorů

As a stavitel (Mlok) i rozhodčí spoléhající na Stanici (implicitní předpoklad všech UJ),
I want aby elektronika, senzory a magnetometr byly v ochranném pouzdře a bezpečně upevněné na rozhodcovské lodi,
So that Stanice přežije stříkající vodu, déšť a běžný pohyb lodi po celý závodní den.

**Acceptance Criteria:**

**Given** ESP32 + elektronika je umístěná v ochranném pouzdře (očekávaně 3D tištěné) na palubě lodi
**When** dojde k typickému stříkání vody nebo dešti během provozu (ne ponoření)
**Then** elektronika uvnitř zůstává suchá a funkční (FR-13)

**Given** anemometr, korouhev a magnetometr jsou upevněné (očekávaně na vrcholu dřevěného stožárku, viz otevřená otázka umístění magnetometru v architektuře/addendu)
**When** loď se za kotvení pohybuje a posádka s ní běžně manipuluje
**Then** upevnění senzorů vydrží bez uvolnění po dobu závodního dne (FR-13)

**Given** umístění magnetometru (stožárek vs. paluba u ESP) je otevřená otázka z PRD/architektury
**When** tato story se implementuje
**Then** rozhodnutí o umístění je zaznamenané zde jako splněný krok (i-drát I2C délka ověřena prakticky), ne znovu otevírané — je to mechanická/elektro otázka mimo softwarovou architekturu

---

## Validace pokrytí (Step 4)

**FR pokrytí:** Všech 15 FR (FR-1 až FR-15) je pokryto alespoň jednou story — viz FR Coverage Map výše. Žádné FR nezůstává nepokryté.

**Nezávislost epik:** Epic 1 je kompletně funkční samostatně (FR-3 už obsahuje auto-reconnect, takže "živé čtení" funguje i bez Epic 2 resilience vrstvy — jen bez odolnosti proti déletrvajícím výpadkům). Epic 2 staví na Epic 1, ale Epic 1 nepotřebuje Epic 2 k fungování. Epic 3 staví na úložišti z Epic 1 a na WS eventu z Epic 2 (Story 2.3 vysílá `history-changed`, i když do Epic 3 nemá posluchače — harmless, ověřeno v AC Story 2.3). Epic 4 a Epic 5 jsou nezávislé na Epic 2/3, staví jen na existenci Stanice/firmwaru z Epic 1.

**Bez dopředných závislostí uvnitř epiky:** Ověřeno story po story — každá story používá jen výstupy story s nižším číslem ve své epice (viz pořadí 1.1→1.6, 2.1→2.5, 3.1→3.3, 4.1→4.2, 5.1→5.2 popsané v textu výše).

**Databáze/entity jen když potřeba:** Tabulka `readings` vzniká v Story 1.3 (první story, která ji skutečně potřebuje), ne v samostatném "DB setup" kroku. Žádná další entita v systému není potřeba (AD-3: jedno úložiště).

**Starter template:** Architektura nepředepisuje starter template — Story 1.3 zakládá backend projekt jako součást prvního užitečného endpointu.

**File churn kontrola:** Epic 1 a Epic 2 obě upravují `firmware/src/transmit` a `backend/src/ingest` — posouzeno jako smysluplné rozdělení (ne náhodné překrývání): Epic 1 dodává minimální živý přenos (uživatelská hodnota #1 dle PRD priority), Epic 2 je vědomě oddělená vrstva odolnosti přidaná AŽ POTÉ, co základní přenos funguje a je ověřený — to odpovídá PRD prioritě "funkční živé čtení" před vším ostatním a dává smysl jako samostatný feedback loop (nejdřív ověřit, že živý přenos vůbec funguje na vodě, pak přidat odolnost). Konsolidace do jedné epiky by skryla tuto prioritizaci.

## Poznámka k rozsahu tohoto běhu

Tento breakdown byl proveden jako autonomní běh (Mlok formuloval zadání/prioritu v briefu k tomuto úkolu; žádná živá interaktivní elicitace přes menu neproběhla, protože běh probíhal bez připojeného interaktivního uživatele). Rozhodnutí o struktuře epik/stories vycházejí přímo z PRD (kap. 6.2 priorita, FR-1–15) a Architecture Spine (Capability→Architecture Map, AD-1 až AD-10) beze změny jejich obsahu — jde o mechanický rozklad, ne o nová produktová rozhodnutí. Otevřené otázky, které PRD/architektura už označily jako otevřené (fyzický přepínač Wi-Fi, umístění magnetometru, přesná kapacita bufferu, přesný vzorkovací interval), zůstávají otevřené i zde a jsou vyznačené v příslušných story ACs — nejsou to nové otázky vzniklé tímto krokem.
