---
title: Gustik
status: final
created: 2026-08-01
updated: 2026-08-01
---

# PRD: Gustik
*Working title — potvrdit.*

## 0. Účel dokumentu

Tento PRD popisuje první verzi (v1) meteostanice Gustik pro skautské jachtařské regaty — pro Mloka jako vývojáře a stavitele, případně pro Gurua při pomoci s elektromechanikou. Navazuje na `brief.md` a `addendum.md` (`_bmad-output/planning-artifacts/briefs/brief-gustik-2026-08-01/`) — nezopakovává jejich obsah, ale destiluje ho do ověřitelných požadavků. Technické detaily nasazení (backend infrastruktura, fyzické zapojení) jsou zachycené v `addendum.md` tohoto PRD běhu, ne přímo zde. Dokument je strukturovaný kolem glosáře pojmů (kap. 3), funkcí seskupených s vnořenými funkčními požadavky (FR, kap. 4) a inline `[ASSUMPTION]` značek tam, kde jsem si detail domyslel bez explicitního potvrzení — všechny jsou sesbírané v kap. 10.

## 1. Vize

Gustik je meteostanice na míru pro startovní/rozhodcovský člun na skautských jachtařských regatách (třídy P550, Optimist, Topaz). Měří rychlost a směr větru přímo na hladině, u startovní lodi — ne na břehu, protože na malých rybnících se vítr na břehu a na vodě citelně liší. Data proudí přes Wi-Fi na webový dashboard, kde je vidí rozhodčí i vedoucí oddílů živě i v historickém grafu.

Řeší bezpečnostní rozhodnutí s reálnými důsledky: třída P550 se v silném větru může převrátit, a rozhodčí dnes toto rozhodnutí dělá odhadem od oka. Gustik jim dá číslo a směr, kterému mohou věřit, místo odhadu.

Stejně důležitý je druhý cíl projektu: postavit si stanici vlastníma rukama jako DIY projekt, s využitím senzorů ze staré meteostanice WH1080/WH1090, ESP32 a magnetometru pro korekci směru vůči stáčení kotvící lodi. Funkčnost má přednost před dokonalostí — hrubé hrany v v1 jsou přijatelné, pokud je stanice postavená a nasazená a **dá se provozovat i bez toho, aby byl Mlok osobně na lodi každé ráno**.

Pokud by DIY přístup u konkrétní součástky reálně ohrozil termín 17. 8. 2026, priorita je nasadit do termínu — v tom případě je v pořádku danou část nahradit hotovým řešením, ne slevit z termínu kvůli principu "postavit si to sám".

## 2. Cílový uživatel

### 2.1 Jobs To Be Done

- **Jako hlavní rozhodčí regaty** potřebuji vědět aktuální rychlost a směr větru na vodě, abych mohl rozhodnout, jestli je bezpečné pokračovat v závodění (zejména u P550).
- **Jako hlavní rozhodčí** potřebuji zpětně vidět, jak se vítr měnil během dne, abych mohl vyhodnotit průběh regaty a plánovat další ročníky.
- **Jako vedoucí oddílu/družiny** potřebuji stejná živá data, abych mohl posádky před dalším rozjezdem instruovat (např. jiné nastavení plachet, zvýšená opatrnost).
- **Jako stavitel (Mlok)** chci si stanici postavit sám z dostupných součástek — to je součást cíle, ne jen prostředek k němu.
- **Jako stavitel (Mlok)** chci, aby po prvním dni akce dokázal stanici ráno zprovoznit i někdo jiný z rozhodčích podle psaných pokynů — nechci muset být na lodi osobně každé ráno, jen při počátečním nastavení.

### 2.2 Ne-uživatelé (v1)

- Rozhodčí a vedoucí **jiných** regat/oddílů, které tuto konkrétní stanici nepoužívají — v1 je jedna stanice pro jednu konkrétní regatu, ne sdílená služba.
- Diváci/veřejnost bez přímého zájmu na rozhodování o závodění — dashboard není budovaný jako veřejná prezentace.
- Posádky lodí samotné za plavby (na vodě nemají k dashboardu přístup) — data čtou organizátoři a vedoucí na břehu/lodi rozhodčích, ne závodníci za jízdy.

### 2.3 Klíčové uživatelské scénáře

- **UJ-1. Tomáš, hlavní rozhodčí, rozhoduje o pokračování závodění.**
  Tomáš sedí na rozhodcovském člunu mezi rozjezdy P550. Otevře na mobilu dashboard Gustiku (veřejný odkaz, bez přihlašování), uvidí aktuální rychlost a směr větru. Vítr zesílil nad hranici, kterou si pro P550 stanovili předem. Rozhodne se další rozjezd P550 odložit a pustit jen Optimisty. **Edge case:** pokud stanice právě ztratila spojení, pozná to jednak z LED na jednotce (FR-5), jednak přímo na dashboardu, kde je aktuální hodnota označená jako zastaralá (FR-11) — ví, že zobrazené číslo může být neaktuální, a rozhoduje o to opatrněji.

- **UJ-2. Klára, vedoucí oddílu, brífuje posádku před rozjezdem.**
  Klára čeká na startu s dětmi z oddílu, mimo dohled na loď rozhodčích. Na svém telefonu otevře stejný dashboard jako Tomáš a uvidí aktuální směr a sílu větru. Podle toho poradí posádce úpravu plachet a připomene zvýšenou opatrnost. Realizuje stejné JTBD jako UJ-1, jen z pohledu vedoucího místo rozhodčího. **Edge case:** LED na Stanici Klára z lodi rozhodčích nevidí — pokud spojení vypadlo, spolehne se na indikaci zastaralosti dat přímo na dashboardu (FR-11), ne na fyzický signál na jednotce.

- **UJ-3. Tomáš zpětně vyhodnocuje průběh dne.**
  Po skončení závodů otevře historický graf a prohlédne si, jak se vítr měnil mezi jednotlivými rozjezdy — použije to při plánování rozpisu na další den nebo příští ročník. Srovnání je manuální (čtení grafu), ne dedikovaná funkce pro porovnání rozjezdů.

- **UJ-4. Petra, rozhodčí, sama zprovozní stanici druhý den akce.**
  Je druhé ráno Plachetního soustředění na Nechanicích. Mlok už není na místě. Petra podle psaných instrukcí na webu zapojí Stanici do powerbanky, případně přepne přepínač podle toho, jestli bude fungovat břehový hotspot nebo mobilní hotspot na jejím telefonu, počká, až LED ukáže úspěšné připojení, a zkontroluje na dashboardu, že se objevují čerstvá data. Žádný notebook, žádné zadávání hesel, žádné přeprogramovávání — jen zapojení, sledování LED a případný přepínač. Realizuje JTBD "Mlok nechce být na lodi každé ráno".

## 3. Glosář

- **Stanice** — fyzická jednotka na startovním/rozhodcovském člunu: ESP32-WROOM + anemometr a větrná korouhev (ze senzorů WH1080/WH1090) + magnetometr HMC5883L + napájení z powerbanky. Jedna stanice na jednu regatu v v1.
- **Dashboard** — webová aplikace zobrazující aktuální hodnoty a historický graf; přístupná z mobilu i PC bez přihlašování.
- **Backend** — webová/cloudová služba přijímající data ze Stanice a obsluhující Dashboard (nasazení a interní architektura viz `addendum.md`).
- **Rozhodčí/Organizátor** — osoba rozhodující o bezpečnosti a průběhu závodění na základě dat ze Stanice; v1 je zároveň tím, kdo Stanici ráno zprovozňuje.
- **Vedoucí oddílu** — vedoucí skautského oddílu/družiny, používá stejný Dashboard k instruktáži posádek.
- **Stáčení (yaw)** — otáčení kotvící rozhodcovské lodi kolem své osy (loď se nepřemisťuje, jen se stáčí); Magnetometr koriguje naměřený směr větru o toto stáčení.
- **Lokální buffer** — dočasné úložiště naměřených dat přímo na Stanici, použité při výpadku spojení se Backendem; při obnovení spojení se doplní (backfill) do Backendu.
- **Výpadek spojení** — stav, kdy Stanice nemůže odeslat data na Backend (např. mimo dosah Wi-Fi); signalizován LED na Stanici.
- **RSSI** — síla přijímaného Wi-Fi signálu, měřená Stanicí, použitá k vyhodnocení dosahu Wi-Fi na místě.
- **Zprovoznění (provisioning)** — ranní proces uvedení Stanice do provozu (zapnutí, připojení k Wi-Fi, ověření na dashboardu), navržený tak, aby ho zvládl kdokoli z rozhodčích bez zásahu do firmwaru.
- **Regata** — jedna konkrétní skautská jachtařská soutěž; v1 cíl je Plachetní soustředění na Nechanicích (YC4PVS), start 17. 8. 2026.
- **Rozjezd (heat)** — jedno kolo závodění v rámci Regaty; Gustik v1 rozjezdy explicitně netaguje ani strukturovaně neporovnává (viz Nezahrnuto).

## 4. Funkce

### 4.1 Měření větru

**Popis:** Stanice měří rychlost a směr větru pomocí senzorů zachráněných z WH1080/WH1090 (anemometr na reed spínač, korouhev na odporový dělič), čtených přes ESP32. Protože rozhodcovská loď kotví a stáčí se, je surový směr z korouhve zkreslený — magnetometr HMC5883L měří aktuální natočení lodi a Stanice tímto natočením surový směr koriguje. Realizuje UJ-1, UJ-2.

**Funkční požadavky:**

#### FR-1: Měření rychlosti větru

Stanice měří okamžitou rychlost větru z anemometru.

**Důsledky (testovatelné):**
- Stanice počítá pulsy z reed spínače anemometru a převádí je na rychlost větru v definovaném intervalu vzorkování. `[ASSUMPTION: interval vzorkování ~2–5 s, přesná hodnota bude doladěna podle chování senzoru a UX dashboardu.]`
- Interně se rychlost počítá a ukládá v m/s (potvrzeno uživatelem jako český meteorologický standard); zobrazení v jiné jednotce řeší dashboard (FR-10), ne měření samotné.

#### FR-2: Měření směru větru s korekcí na stáčení lodi

Stanice měří relativní směr větru z korouhve a koriguje ho o aktuální natočení lodi naměřené magnetometrem, aby výsledný směr byl vztažený k severu, ne k přídi lodi.

**Důsledky (testovatelné):**
- Korouhev ze senzoru WH1080/WH1090 dává směr jen v rozlišení **8 kardinálních směrů** (po 45°) — korigovaný výsledek se po přičtení natočení lodi zaokrouhlí na nejbližší z těchto 8 směrů, ne na přesný stupeň.
- Přesnost magnetometru na úrovni jednotek stupňů je vzhledem k tomuto hrubému rozlišení zanedbatelná — potvrzeno uživatelem: pro rozhodování je síla větru mnohem důležitější než přesný směr.
- Výsledný směr větru se nemění jen proto, že se kotvící loď otočí — magnetometr korekci provádí průběžně.
- Magnetometr vyžaduje kalibraci (hard-iron/soft-iron) před nasazením na konkrétní loď; korekce je určena pro fiberglassové neplechové kotvící plavidlo, ne pro kovovou loď.
- Náklon lodi (tilt) se v v1 nekompenzuje — magnetometr nemá vestavěnou kompenzaci náklonu. `[ASSUMPTION: přijatelné riziko, protože rozhodcovský člun je stabilní a náklon za kotvení malý; plná IMU fúze je v2.]`

**Mimo rozsah:**
- Kompenzace náklonu (tilt) přes akcelerometr/IMU fúzi — v2.
- GPS-based heading nebo GNSS kompas — nefunguje za kotvení, zvažováno jen jako budoucí alternativa.
- Přesnost směru jemnější než 8 kardinálních směrů — daná rozlišením senzoru, ne architektonickým rozhodnutím.

### 4.2 Konektivita a odolnost

**Popis:** Stanice odesílá naměřená data přes Wi-Fi na Backend. Preferovaná varianta je hotspot na břehu; jako záloha může sloužit mobilní hotspot z telefonu jednoho z rozhodčích na lodi. Pokud spojení vypadne, data se bufferují lokálně a po obnovení spojení se doplní. Ztráta spojení je signalizovaná LED přímo na Stanici. Realizuje UJ-1 (edge case).

**Funkční požadavky:**

#### FR-3: Odesílání dat přes Wi-Fi

Stanice se připojuje k dostupné Wi-Fi síti (hotspot na břehu, nebo záložně mobilní hotspot na lodi) a periodicky odesílá naměřená data na Backend.

**Důsledky (testovatelné):**
- Stanice funguje v dosahu, který bude ověřen přímo na místě (Nechanice) — viz FR-6 (logování RSSI) a Otevřené otázky.
- Při ztrátě signálu se Stanice pokouší o opětovné připojení, aniž by vyžadovala restart nebo zásah obsluhy.

#### FR-4: Lokální buffer a doplnění dat (backfill)

Při výpadku spojení Stanice ukládá naměřená data lokálně a po obnovení spojení je odešle na Backend, aby v historii nevznikla díra.

**Důsledky (testovatelné):**
- Stanice udrží v lokálním bufferu alespoň **4 hodiny** naměřených dat při obvyklé frekvenci vzorkování, než začne nejstarší data přepisovat. `[ASSUMPTION: 4 h zvoleno jako reprezentant "několika hodin" potvrzených uživatelem; přesnou hodnotu doladit podle skutečné kapacity flash paměti ESP32 a intervalu vzorkování z FR-1.]`
- Jde o best-effort mechanismus — garantováno není nic, cílem je pokrýt běžný výpadek na vodě, ne libovolně dlouhý výpadek.
- Po obnovení spojení se bufferovaná data doplní do historie na Backendu ve správném časovém pořadí.

**Mimo rozsah:**
- Garance nulové ztráty dat při výpadku delším než buffer — mimo rozsah v1.

#### FR-5: Signalizace ztráty spojení

Stanice signalizuje ztrátu spojení se Backendem přímo na jednotce (např. LED), aby posádka na lodi věděla, že se aktuálně nezobrazují živá data.

**Důsledky (testovatelné):**
- Signalizace se aktivuje do **10 vteřin** od zjištění výpadku a zhasne/změní stav po obnovení spojení.

#### FR-6: Logování síly Wi-Fi signálu (RSSI)

Stanice zaznamenává sílu přijímaného Wi-Fi signálu spolu s naměřenými daty, aby šlo po akci vyhodnotit reálně dosažitelný dosah z břehu.

**Důsledky (testovatelné):**
- Každý odeslaný/bufferovaný záznam obsahuje aktuální RSSI v okamžiku měření.
- Hodnoty RSSI jsou po akci dohledatelné (na dashboardu, nebo alespoň v exportovatelném logu) pro účely vyhodnocení dosahu — přesná forma zobrazení je otevřená (viz Otevřené otázky).

### 4.3 Dashboard

**Popis:** Webová aplikace zobrazující aktuální rychlost a směr větru a historický graf, dostupná z mobilu i PC bez přihlašování. Stejný pohled pro rozhodčí i vedoucí oddílů — žádné role ani oprávnění v v1. Realizuje UJ-1, UJ-2, UJ-3.

**Funkční požadavky:**

#### FR-7: Zobrazení aktuální hodnoty

Dashboard zobrazuje aktuální rychlost a směr větru, čitelné na první pohled.

**Důsledky (testovatelné):**
- Hodnota se na Dashboardu aktualizuje bez nutnosti ručního obnovení stránky (např. automatický refresh nebo push).
- Layout je použitelný jak na mobilním telefonu, tak na PC (responzivní).

#### FR-8: Historický graf

Dashboard zobrazuje graf vývoje rychlosti a směru větru v čase, umožňující zpětně srovnat podmínky mezi různými částmi dne.

**Důsledky (testovatelné):**
- Graf zobrazuje minimálně data z aktuálního závodního dne.
- Uživatel může v grafu vizuálně identifikovat období silnějšího/slabšího větru bez dedikované funkce pro tagování rozjezdů.

**Mimo rozsah:**
- Strukturované tagování a porovnávání jednotlivých rozjezdů — mimo rozsah v1 (viz Nezahrnuto).

#### FR-9: Veřejný přístup bez přihlašování

Dashboard je dostupný přes sdílený odkaz bez nutnosti přihlášení nebo hesla.

**Důsledky (testovatelné):**
- Kdokoli se znalostí URL vidí stejná data — žádné role, účty ani odlišná oprávnění pro rozhodčí vs. vedoucí.

**Poznámky:** Rozhodnuto s uživatelem v Discovery — veřejný odkaz preferován před heslem kvůli nulové zátěži navíc a odpovídá stavu "žádné role v v1" z briefu.

#### FR-10: Přepínání jednotky rychlosti větru

Dashboard umožňuje zobrazit rychlost větru v m/s nebo v uzlech podle volby uživatele.

**Důsledky (testovatelné):**
- Přepínač jednotky je dostupný přímo na Dashboardu (aktuální hodnota i historický graf).
- Interní výpočet a ukládání dat zůstává v m/s (FR-1); uzly jsou čistě zobrazovací převod, ne alternativní způsob měření.

#### FR-11: Indikace stáří dat na dashboardu

Dashboard viditelně ukazuje, jak stará jsou aktuálně zobrazená data, aby i vzdálený divák bez výhledu na Stanici (ne jen někdo stojící u LED na lodi) poznal, že se dívá na potenciálně neaktuální hodnotu. Realizuje UJ-1 (edge case), UJ-2 (edge case).

**Důsledky (testovatelné):**
- Dashboard zobrazuje čas posledního přijatého záznamu u aktuální hodnoty (např. "před X s/min").
- Pokud nepřišla nová data déle než definovaná hranice, je aktuální hodnota vizuálně odlišená jako potenciálně neplatná (např. přeškrtnutí, změna barvy). `[ASSUMPTION: hranice "zastaralé" stanovena na 2 minuty bez nových dat; nezávislé na 10s prahu LED signalizace na Stanici (FR-5), který řeší jiný scénář (fyzická přítomnost u jednotky).]`
- Tento indikátor funguje nezávisle na LED (FR-5) — pokrývá diváky, kteří nejsou fyzicky u Stanice.

### 4.4 Napájení

**Popis:** Stanice běží celý závodní den na jedno nabití USB-C powerbanky, dobíjí se přes noc. Realizuje předpoklad UJ-1–UJ-4.

**Funkční požadavky:**

#### FR-12: Provoz na powerbanku po celý závodní den

Stanice je napájená z USB-C powerbanky a vydrží nepřerušený provoz po dobu jednoho závodního dne.

**Důsledky (testovatelné):**
- Stanice po plném nabití běží nepřetržitě minimálně **8 hodin** (typický plachetní den cca 9:00–17:00), bez nutnosti výměny nebo dobití powerbanky během dne.
- Powerbanka se dobíjí přes noc mezi závodními dny.

### 4.5 Mechanická instalace

**Popis:** Senzory, magnetometr a elektronika jsou umístěné v ochranném pouzdře (očekávaně 3D tištěné) a bezpečně upevněné na rozhodcovském člunu tak, aby vydržely stříkající vodu a déšť. Meteo senzory (anemometr, korouhev) jsou plánované na vrcholu dřevěného stožárku pro čistší měření; ESP32 s powerbankou na palubě lodi. Realizuje předpoklad všech UJ.

**Funkční požadavky:**

#### FR-13: Ochranné pouzdro a upevnění na loď

Elektronika a senzory jsou chráněné před stříkající vodou a deštěm a bezpečně upevněné na rozhodcovské lodi po dobu závodního dne.

**Důsledky (testovatelné):**
- Pouzdro odolá stříkající vodě a dešti během typického provozu na regatě (ne ponoření).
- Upevnění senzorů a magnetometru vydrží běžný pohyb lodi a manipulaci posádky bez uvolnění.

**Poznámky:** Umístění magnetometru (u meteo senzorů na stožárku vs. u ESP na palubě) je otevřené — viz Otevřené otázky a §11.1 Hardwarová omezení (kompromis délky I2C sběrnice).

### 4.6 Nastavení a zprovoznění

**Popis:** Nová oblast identifikovaná při review draftu: bez ní by zprovoznění Stanice každé ráno vyžadovalo přítomnost Mloka a případně přeprogramování ESP kvůli změně Wi-Fi sítě. Cílem je, aby od druhého dne akce zvládl ranní zprovoznění kdokoli z rozhodčích, bez zásahu do firmwaru a bez volání Mlokovi. Realizuje UJ-4.

**Funkční požadavky:**

#### FR-14: Napevno nakonfigurované Wi-Fi sítě, bez interakce obsluhy v terénu

Stanice má napevno nakonfigurované přihlašovací údaje pro jednu nebo dvě známé Wi-Fi sítě (břehový hotspot, záložní mobilní hotspot) a při zapnutí se k dostupné z nich připojí sama, bez jakéhokoli zásahu obsluhy.

**Důsledky (testovatelné):**
- Rozhodčí na lodi se Stanicí nijak nekonfiguračně neinteraguje — žádný notebook, žádné zadávání hesel, žádné nastavování. Jediná fyzická interakce je zapojení do powerbanky, sledování LED (FR-5) a případně mačkání tlačítek/přepínačů (viz níže).
- Pokud se v praxi ukáže potřebné ručně vybrat mezi břehovou a mobilní sítí (např. obě jsou v dosahu, ale jen jedna má funkční internet), Stanice může mít jednoduchý fyzický přepínač pro tuto volbu. `[ASSUMPTION: potřeba fyzického přepínače se potvrdí/zamítne v elektro fázi — pokud automatický fallback mezi oběma sítěmi funguje spolehlivě, přepínač nemusí být nutný.]`
- Změna SSID/hesla uložených ve Stanici (např. před další akcí) vyžaduje fyzické připojení notebooku (sériová linka nebo USB) a je to servisní zásah — dělá ho Mlok, ne rozhodčí v terénu. Konfigurace je uložená v jednoduchém souboru editovatelném bez nutnosti přeflashovat celý firmware.

**Mimo rozsah:**
- Automatické vyhledávání a připojování k neznámým, předem nenastaveným sítím.
- Jakékoli UI/rozhraní pro konfiguraci Wi-Fi dostupné rozhodčímu v terénu (captive portal apod.) — konfigurace je čistě servisní úkon mimo den akce.

#### FR-15: Psané instalační a provozní pokyny publikované na webu

Existuje psaný návod, dostupný na webu (ideálně provázaný s Dashboardem), popisující ranní zprovoznění Stanice a základní fyzickou instalaci, srozumitelný pro rozhodčího bez technického zázemí.

**Důsledky (testovatelné):**
- Návod pokrývá alespoň: zapnutí Stanice a powerbanky, co dělat s případným přepínačem břeh/mobil (FR-14), jak poznat úspěšné připojení (LED, FR-5), jak ověřit na Dashboardu, že přicházejí čerstvá data (FR-11), a co dělat, když se něco z toho nepovede.
- Pokud se použije záložní mobilní hotspot, návod uvádí přesné SSID a heslo, které si má rozhodčí na svém telefonu nastavit — musí se shodovat s tím, co je předem nahrané ve Stanici (viz FR-14), protože Stanice se v terénu nepřekonfigurovává.
- Návod je dostupný online (ne jen v hlavě Mloka nebo v repozitáři kódu) tak, aby si ho rozhodčí na místě mohl znovu otevřít bez přístupu k vývojářskému prostředí.

**Poznámky:** Přesné umístění (samostatná stránka na Dashboardu vs. jinde) je otevřené — viz Otevřené otázky.

## 5. Nezahrnuto (explicitně)

- Více stanic na jedné regatě (např. srovnání startu vs. trati) — future vision, ne v1.
- Jedna webová aplikace agregující data napříč více regatami/sezónami — future vision, ne v1.
- Strukturované tagování rozjezdů a dedikovaná funkce pro jejich porovnání — historie zůstává v v1 čitelná jen jako graf.
- Automatické alerty/notifikace při překročení bezpečnostního prahu větru — rozhodnutí "bezpečné/nebezpečné" zůstává v v1 čistě na lidském úsudku nad zobrazeným číslem, ne na automatizaci.
- Sdílená obrazovka/kiosk režim pro zobrazení dashboardu — zmíněno v briefu jako možné budoucí rozšíření, ne požadavek v1.
- Role a oprávnění uživatelů — dashboard je jeden veřejný pohled pro všechny.
- Alternativní konektivita (LoRa, mobilní data přímo ze Stanice) jako záloha za Wi-Fi — mimo rozsah v1; jediná zmírňující opatření jsou lokální buffer (FR-4) a záložní mobilní hotspot z telefonu rozhodčího (FR-3).
- Automatické vyhledávání neznámých Wi-Fi sítí mimo předkonfigurovaný seznam (FR-14).

## 6. Rozsah MVP

### 6.1 Zahrnuto

Jedna Stanice na Plachetním soustředění na Nechanicích (start 17. 8. 2026): měření větru s yaw-korekcí (FR-1–2), Wi-Fi přenos s bufferem a RSSI logem (FR-3–6), veřejný dashboard s live hodnotou, historií a indikací stáří dat (FR-7–11), celodenní provoz na powerbance (FR-12), voděodolná instalace (FR-13) a zprovoznění bez Mloka od druhého dne (FR-14–15).

### 6.2 Mimo rozsah MVP

- Vše uvedené v kap. 5 Nezahrnuto.
- Přesná kompenzace náklonu (tilt) magnetometru přes IMU fúzi — `[NOTE FOR PM: pokud čas do 17. 8. zbyde, zvážit alespoň hrubou akcelerometrickou korekci — addendum ji zmiňuje jako levné rozšíření. Nízká priorita vzhledem k tomu, že rozlišení směru je stejně jen 8 kardinálních směrů.]`
- OTA (over-the-air) aktualizace firmwaru — `[ASSUMPTION: v1 firmware se nahrává kabelem před nasazením, ne vzdáleně; Wi-Fi přihlašovací údaje se ale dají měnit bez přeflashování, viz FR-14.]`
- Volba a konfigurace konkrétní backendové technologie (framework, databáze, frontend) — architektonické rozhodnutí; nasazovací platforma (Docker/Compose za existující Caddy proxy) je už daná, viz `addendum.md`.

**Priorita při časovém tlaku** (převzato z briefu, platí i zde): funkční živé čtení > historický graf > vyladěná mechanika.

## 7. Metriky úspěchu

**Primární**
- **SM-1**: Stanice je nasazená a používaná na Plachetním soustředění na Nechanicích od 17. 8. 2026. Validuje FR-1 až FR-15.
- **SM-2**: Stanice vydrží celý závodní den (≥8 h) na jedno nabití powerbanky bez výpadku napájení. Validuje FR-12.
- **SM-3**: Rozhodčí i vedoucí vidí aktuální rychlost a směr větru na dashboardu živě, z mobilu i PC, a poznají, pokud jsou data zastaralá. Validuje FR-7, FR-9, FR-11.

**Sekundární**
- **SM-4**: Dashboard umožňuje zpětně zobrazit historii větru přes celý závodní den, ne jen aktuální hodnotu. Validuje FR-8.
- **SM-5**: Mechanická instalace (pouzdro, upevnění senzorů) vydrží celý den provozu bez selhání. Validuje FR-13.
- **SM-6**: Od druhého dne akce dokáže Stanici ráno samostatně zprovoznit rozhodčí na lodi, bez přítomnosti nebo zásahu Mloka, podle psaných pokynů. Validuje FR-14, FR-15.
- **SM-7**: Po akci je z RSSI logu zřejmé, jaký dosah Wi-Fi z břehu byl reálně dosažitelný — použitelné pro plánování příští instalace. Validuje FR-6.

**Kontra-metriky (neoptimalizovat)**
- **SM-C1**: Honba za maximální přesností směru větru na úkor stihnutí termínu 17. 8. 2026 — brief explicitně preferuje funkční nad dokonalé, navíc senzor stejně dává jen 8 kardinálních směrů. Vyvažuje SM-1.
- **SM-C2**: Snaha o garanci nulové ztráty dat při výpadku spojení na úkor jednoduchosti implementace — FR-4 je vědomě best-effort, ne garance. Vyvažuje SM-4.

## 8. Proč teď

Termín nasazení je **17. 8. 2026** — Plachetní soustředění na Nechanicích ([YC4PVS](https://yc4pvs.skauting.cz/index.php/aktualni-akce/)). Od data tohoto PRD (1. 8. 2026) zbývá přesně **16 dní**. Regatový kalendář diktuje datum, ne technická připravenost: pokud stanice nebude hotová na tuto akci, nejbližší další příležitost k reálnému otestování v provozu je až příští sezóna. To je hlavní důvod, proč se priorita v celém dokumentu (kap. 6.2, kap. 7) opakovaně kloní k "funkční nad dokonalé" a proč jsou v2 položky (tilt kompenzace, OTA, multi-stanice) vědomě odsunuté, ne zapomenuté.

Časový tlak násobí i to, že na projektu v principu pracuje jeden vývojář (Mlok), který pokrývá software i mechaniku. Guru (skautský bratr) by rád pomohl s elektromechanickou částí (pájení, umístění senzorů) a jeho pomoc by termín usnadnila, ale plán s ní nepočítá jako s jistotou — FR-13 (ochranné pouzdro a upevnění) je naplánováno tak, aby ho zvládl Mlok i sám.

## 9. Otevřené otázky

1. **Skutečně dosažitelný dosah Wi-Fi z břehu na Nechanicích** — bude měřeno přímo na místě; FR-6 (logování RSSI) má poskytnout data pro vyhodnocení. Pokud dosah nebude stačit, záložní varianta je mobilní hotspot na lodi (FR-3) — v tom případě zůstává otevřená otázka výdrže baterie telefonu použitého jako hotspot po celý den.
2. **Konkrétní backendová technologie** (framework, databáze, frontend) v rámci již zvoleného nasazení (Docker/Compose za existující Caddy reverzní proxy s HTTPS) — viz `addendum.md`, řešit v architektuře.
3. **Umístění magnetometru** — u meteo senzorů na vrcholu stožárku (vyžaduje dlouhou I2C sběrnici k ESP) vs. u ESP na palubě lodi — technické rozhodnutí pro mechanickou/elektro fázi, viz `addendum.md` a §11.1.
4. **Přesná kapacita lokálního bufferu** (FR-4) — cíl 4 h je zástupná hodnota, k doladění podle skutečné paměti ESP32 a intervalu vzorkování.
5. **Přesné umístění psaných instalačních pokynů** (FR-15) — samostatná stránka na Dashboardu, nebo jinde na webu.
6. **Potřeba fyzického přepínače pro volbu mezi břehovým a mobilním hotspotem** (FR-14) — vyjasní se v elektro fázi podle toho, jak spolehlivě funguje automatický fallback mezi oběma předkonfigurovanými sítěmi.
7. **Dostupnost Gurua** pro elektromechanickou pomoc je nejistá (logistika) — pokud pomůže, může to zkrátit čas na FR-13/mechanickou fázi; plán s tím ale nepočítá jako s jistotou (viz kap. 8).

## 10. Index předpokladů

- §4.1 FR-1 — interval vzorkování rychlosti větru ~2–5 s, přesná hodnota neurčená.
- §4.1 FR-2 — kompenzace náklonu (tilt) magnetometru se v v1 nedělá; riziko přijato kvůli stabilitě kotvící lodi.
- §4.2 FR-4 — cíl lokálního bufferu stanoven na 4 hodiny jako reprezentant potvrzeného "několika hodin"; přesná hodnota k doladění podle kapacity paměti.
- §4.3 FR-11 — hranice "zastaralých dat" na dashboardu stanovena na 2 minuty bez nových dat; nezávislá na 10s prahu LED (FR-5).
- §4.6 FR-14 — potřeba fyzického přepínače mezi břehovou a mobilní sítí není potvrzená; závisí na spolehlivosti automatického fallbacku.
- §6.2 — firmware se v v1 aktualizuje kabelem, Wi-Fi přihlašovací údaje jdou měnit bez přeflashování (FR-14).

---

## Doplňkové sekce (Adapt-In)

## 11. Průřezová omezení

### 11.1 Hardwarová omezení

- **MCU**: ESP32-WROOM — určuje dostupný výpočetní výkon, paměť pro lokální buffer (FR-4) a spotřebu energie.
- **Senzory**: anemometr a korouhev znovupoužité z WH1080/WH1090 — omezují na jejich nativní rozlišení (8 kardinálních směrů, FR-2) a mechanické provedení; posouzeno v addendu jako nízkoriziková, "vyšlapaná cesta".
- **Magnetometr**: HMC5883L — bez vestavěné kompenzace náklonu, vyžaduje hard-iron/soft-iron kalibraci a lokální magnetickou deklinaci; degraduje blízko kovu/elektroniky. Plánované umístění (na stožárku vs. u ESP) ovlivňuje délku I2C sběrnice — delší sběrnice od stožárku k ESP je elektricky náročnější než umístění blízko ESP (viz Otevřené otázky, `addendum.md`).
- **Napájení**: musí pokrýt ESP32 + senzory + Wi-Fi rádio po celý závodní den (≥8 h) z jedné USB-C powerbanky (FR-12) — spotřeba Wi-Fi vysílání je pravděpodobně dominantní faktor.
- **RSSI**: čtení síly Wi-Fi signálu (FR-6) je standardní součást Wi-Fi API na ESP32 — nízké technické riziko.

### 11.2 Nasazení a aktualizace firmwaru

- Firmware se v v1 aktualizuje kabelem, ne OTA (viz kap. 6.2). Wi-Fi přihlašovací údaje jsou konfigurovatelné bez přeflashování celého firmwaru (FR-14), ale stále jen fyzickým připojením notebooku (sériová linka/USB) — servisní úkon prováděný Mlokem mimo den akce, ne rozhodčím v terénu.
- Žádný mechanismus pro vzdálenou diagnostiku/rekonfiguraci Stanice na místě mimo fyzický přístup není v v1 plánován nad rámec FR-14.

### 11.3 Provozní a spolehlivostní požadavky

- Stanice musí fungovat venku, na vodě, za typického letního počasí regaty (slunce, možný déšť, stříkající voda) — viz FR-13.
- Stanice je namontovaná na kotvící, ale stáčející se lodi — ne na pevné konstrukci; mechanické upevnění musí zvládnout běžný pohyb lodi a manipulaci posádky.
- Provoz je omezen na dobu trvání jedné regaty (typicky víkend/několik dní, 9:00–17:00) — stanice se mezi dny demontuje/dobíjí, ne kontinuální 24/7 provoz.
- Od druhého dne akce musí být ranní zprovoznění proveditelné bez přítomnosti Mloka (FR-14, FR-15) — to je provozní požadavek stejné váhy jako technická spolehlivost senzorů.

## 12. Omezení a mantinely — bezpečnost

- Gustik je **podpora rozhodování**, ne automatizovaný bezpečnostní systém — konečné rozhodnutí o pokračování/přerušení závodění dělá vždy člověk (rozhodčí), nikdy stanice sama.
- Vědomě se v v1 nedělají automatické alerty při překročení prahu větru (kap. 5) — riziko falešného pocitu bezpečí z chybějící automatizace je akceptované, protože zobrazené číslo je stále čitelné a rozhodčí ho aktivně sleduje.
- Přesnost měření (rychlost i směr) není certifikovaná ani validovaná proti referenčnímu přístroji — dashboard by měl komunikovat, že jde o orientační, ne přesné meteorologické měření. `[NOTE FOR PM: zvážit v UX kopii dashboardu drobnou poznámku o orientační povaze dat, aby rozhodčí nebrali číslo jako neomylné.]`
