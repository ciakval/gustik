# Addendum: Gustik PRD

Technický detail a mechanismus-úrovně obsah, který nepatří přímo do `prd.md` (kap. 4 popisuje schopnosti, ne implementaci), ale je užitečný pro navazující architekturu (`bmad-architecture`).

## Nasazení backendu

Uživatel (Mlok) provozuje vlastní webový server, na kterém běží více aplikací jako Docker Compose stacky, s Caddy reverzní proxy zajišťující HTTPS terminaci. Z pohledu nasazení je proto přirozené postavit Backend Gustiku jako Docker container (nebo Docker Compose stack), zapojený za stejnou Caddy proxy.

- **Dáno**: Docker/Docker Compose nasazení, HTTPS přes existující Caddy proxy, self-hosted na serveru, který už uživatel spravuje.
- **Otevřené** (řešit v architektuře): konkrétní backendový framework (zvažován např. FastAPI), volba databáze, volba frontendové technologie pro Dashboard.

## Umístění magnetometru — kompromis délky I2C sběrnice

Plánované fyzické umístění: meteo senzory (anemometr, korouhev) na vrcholu dřevěného stožárku; ESP32 + powerbanka na palubě lodi.

Otevřená otázka je umístění magnetometru HMC5883L:
- **Varianta A — magnetometr u meteo senzorů na stožárku.** Logické z hlediska "měří natočení v místě měření větru", ale vyžaduje natažení delší I2C sběrnice od stožárku dolů k ESP na palubě. I2C je citlivé na délku vedení (kapacita sběrnice, rušení) — delší kabeláž zvyšuje riziko nespolehlivého čtení, případně vyžaduje I2C extender/buffer.
- **Varianta B — magnetometr u ESP na palubě.** Krátká I2C sběrnice, jednodušší a spolehlivější zapojení. Nevýhoda: měří natočení lodi v místě ESP, ne přímo v místě senzorů — u malé lodi by měl být rozdíl zanedbatelný, ale není to formálně ověřeno.

Rozhodnutí zůstává otevřené pro mechanickou/elektro fázi; PRD (§4.5, §11.1) na tuto úvahu odkazuje, ale nerozhoduje ji.

## Wi-Fi hotspot — preference a záloha

Dvě provozní varianty pro připojení Stanice k internetu:
1. **Hotspot na břehu** (preferovaná varianta) — bude testován na místě (Nechanice), jaký dosah reálně poskytuje.
2. **Mobilní hotspot na telefonu jednoho z rozhodčích na lodi** (záložní varianta) — dnešní datové tarify a ceny dělají tuto variantu prakticky bezproblémovou z hlediska nákladů/dat, ale nese dva vedlejší důsledky, které PRD nepovyšuje na formální FR, ale stojí za zvážení v instalačních pokynech (FR-15):
   - Výdrž baterie telefonu použitého jako hotspot po celý závodní den (řešení: telefon může být připojený na vlastní powerbanku/nabíječku).
   - Nutnost, aby si obsluha pamatovala tuto variantu jako fallback v instalačních pokynech, ne jako náhodné improvizované řešení na místě.

FR-6 (logování RSSI) v PRD existuje primárně proto, aby po testu na místě šlo objektivně vyhodnotit, zda varianta 1 (břeh) stačí, nebo je varianta 2 (mobilní hotspot) nutná natrvalo.

## Proč stavět, ne koupit hotové řešení

Ruční anemometry pro rozhodčí existují a dají se koupit — řada Kestrel (1000/2000/3000, cca $80–160 / ~€75–150; levnější varianty cca $50/~€45) a **Windie Pro 360**, cílený přímo na rozhodčí při závodech. Žádný z nich ale nedělá kontinuální logování do webového dashboardu s historií a zpětným porovnáním napříč rozjezdy/dny — to je jádro toho, co Gustik přidává navíc oproti koupenému řešení (viz `brief.md`, "Co je jinak").

Druhý, stejně důležitý motiv je, že cílem je **naučit se to postavit sám**, ne jen mít funkční přístroj — to přímo ovlivňuje, co je v v1 přijatelné (hrubé hrany, DIY řešení místo hotových komponent tam, kde to nehrozí termínu, viz PRD §1).

## Srovnatelné projekty (reference pro architekturu)

Žádný nalezený projekt neřeší přesně stejný případ užití (startovní/rozhodcovská loď + baterie + Wi-Fi + cloud), ale několik slouží jako stavební kameny:

- **curiouselectric/WindSensor** (GitHub) — DIY anemometr+korouhev pro ESP32 logger, navržený na nízkou spotřebu (sleep/wake). Nejbližší architektonická analogie.
- **pilotak/WeatherMeters** (GitHub) — Arduino knihovna přímo pro senzory WH1080/WH1090/Sparkfun, interrupt-driven. Přímý precedens pro znovupoužití koupených senzorů.
- **taunusflieger/anemometer** (ESP32-S3, MQTT/AWS) a **alf45tar/Anemometer** (ESP32-C6, deep-sleep) — obecné cloud-connected uzly s nízkou spotřebou, užitečná reference pro odhad spotřeby (viz §11.1 v PRD).
- **timhughes/esp32-yacht-wind-instrument** — nejbližší jachtařský precedens, ale je to displej pro existující NMEA/Signal K přístroje, ne senzor→cloud logger.
- **Hackaday: "Seriously Upgraded Weather Station"** — nahrazuje elektroniku WH1080 za ESP32; přímý precedens pro přístup "znovupoužít senzory z komerční stanice".

Závěr: nic z nalezeného není stavěné přímo pro bezpečnostní rozhodování rozhodčích při závodění — potvrzuje to mezeru na trhu, o kterou se opírá důvod stavět Gustika (viz sekce výše).
