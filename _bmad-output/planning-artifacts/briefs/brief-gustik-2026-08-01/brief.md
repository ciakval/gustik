---
title: "Product Brief: Gustik"
status: draft
created: 2026-08-01
updated: 2026-08-01
---

# Product Brief: Gustik

## Přehled

Gustik je vlastnoručně postavená meteostanice pro skautské jachtařské závody, která měří sílu a směr větru přímo na hladině — na startovní/rozhodčí lodi, ne na břehu. Na malých vodních plochách, na kterých se tyto závody jezdí, se vítr na břehu a na vodě může výrazně lišit, a právě tenhle rozdíl je informace, kterou dnes organizátoři nemají a museli by ji jinak odhadovat od oka.

Data ze stanice (ESP32-WROOM + magnetometr HMC5883L pro korekci směru + senzory ze stanice WH1080/WH1090) se přes Wi-Fi posílají do webového dashboardu, kde je organizátoři a vedoucí vidí v reálném čase — a zpětně, pro srovnání mezi jednotlivými dny a rozjížďkami. Cílem první verze je jedna stanice na jednom závodě, vytvořená a nasazená do poloviny srpna 2026 na konkrétním skautském závodě — i proto, že to je zároveň smysl projektu: postavit si vlastní věc, ne koupit hotovou. `[ASSUMPTION: pro plánování se počítá s jedním vývojářem, který zvládne software i mechanickou/instalační stránku; případná pomoc druhého člověka s elektromechanikou je možný bonus, ne jistota.]`

## Problém

Na skautských jachtařských závodech (primárně třída P550, částečně Optimist a Topaz) rozhodují organizátoři a vedoucí oddílů za chodu o dvou věcech: zda je ještě bezpečné pokračovat a jak připravit lodě na další rozjížďku. P550 nejsou stavěné na silnější vítr a při něm se mohou převracet — takže tohle rozhodnutí má reálné důsledky pro bezpečnost posádek, ne jen pro komfort závodu.

Problém je, že relevantní data — síla a směr větru přímo na vodě, v místě závodu — dnes chybí. Odhad z břehu je nespolehlivý právě kvůli malé ploše vodní hladiny, kde se podmínky na vodě a na břehu liší. Organizátoři tak rozhodují na základě odhadu, ne měření. Chybí také možnost zpětně porovnat, jaký vítr měla rozjížďka X oproti rozjížďce Y — což by se hodilo pro vyhodnocení závodu i pro plánování dalších ročníků.

## Řešení

Meteostanice umístěná přímo na startovní lodi měří sílu a směr větru pomocí senzorů převzatých ze stanice WH1080/WH1090 a je řízená přes ESP32-WROOM. Magnetometr HMC5883L koriguje směr větru podle skutečného natočení lodi (ta se může na kotvě pootáčet, takže syrové čtení z větrné korouhvičky samo o sobě nestačí).

Stanice se připojí přes Wi-Fi (mobilní hotspot na lodi nebo signál z břehu na 100–200 m) a posílá data do webového/cloudového backendu. Tam se ukládají a zobrazují na dashboardu — organizátoři a vedoucí si aktuální stav větru (sílu, směr a graf průběhu v čase) zobrazí na vlastním mobilu nebo PC. `[ASSUMPTION: v první verzi jde o osobní zobrazení na vlastním zařízení — sdílená obrazovka je mimo rozsah, viz sekce Rozsah.]`

Napájení řeší USB-C powerbanka; cíl je vydržet celý závodní den bez dobíjení, s nabíjením přes noc.

## Co je jinak

Hotové řešení koupit lze — ruční anemometry pro rozhodčí (např. Kestrel řada, Windie Pro 360) v cenách zhruba 45–150 €. Žádný z nich ale neumí to, co je tady jádrem věci: kontinuální logování do webového dashboardu s historií a srovnáním napříč rozjížďkami. Jsou to nástroje na jedno odečtení, ne na průběžné sledování a zpětnou analýzu.

Druhá, stejně důležitá motivace je, že cíl projektu není jen mít funkční stanici, ale postavit si ji sami — naučit se to, ne si to koupit. Tohle není vedlejší poznámka, ale spoluurčuje rozsah: `[ASSUMPTION: řešení preferuje jednoduchost a proveditelnost v termínu před dokonalostí — je v pořádku, že v1 má hrubší hrany, pokud to postavil tým sám.]`

## Pro koho

**Organizátoři a rozhodčí závodu** — potřebují aktuální data, podle kterých rozhodnou, jestli je bezpečné pokračovat a jak připravit další rozjížďku. Úspěch pro ně: mají číslo a směr, kterým věří, místo odhadu od oka.

**Vedoucí oddílů** — sledují stejná data, aby mohli připravit své posádky (např. doporučit jiné plachtění, opatrnost) na podmínky, které je čekají na vodě.

`[ASSUMPTION: obě skupiny sledují stejný dashboard se stejnými daty — nejde o oddělené role s různými právy či pohledy.]`

## Kritéria úspěchu

- Stanice funguje celý závodní den na jedno nabití powerbanky.
- Data (síla, směr větru) se v reálném čase zobrazují na dashboardu dostupném z mobilu i PC.
- Dashboard umožňuje podívat se zpět na průběh větru v čase (graf historie), ne jen na aktuální hodnotu.
- Řešení je nasazené a použité na reálném závodě do poloviny srpna 2026.
- Stanice je mechanicky funkční (krabička, uchycení senzorů a magnetometru na lodi) a vlastními silami postavená — funkční je důležitější než dokonalé.

## Rozsah (verze 1)

**V rozsahu:**
- Jedna meteostanice na jedné startovní/rozhodčí lodi.
- Měření síly a směru větru, s korekcí směru podle natočení lodi (magnetometr).
- Přenos dat přes Wi-Fi do webového backendu.
- Webový dashboard: aktuální hodnota + graf historie v čase, přístupný z mobilu i PC.
- Napájení na celý den z powerbanky přes USB-C.
- Mechanické řešení instalace: krabička (očekává se 3D tisk), uchycení senzorů a magnetometru na lodi tak, aby fungovaly spolehlivě a bezpečně.
- Nasazení na jeden konkrétní závod do poloviny srpna 2026.

**Mimo rozsah (vědomě odloženo):**
- Více meteostanic na jednom závodě.
- Jeden web agregující data z více závodů najednou.
- Explicitní označování a strukturované porovnávání jednotlivých rozjížděk v dashboardu (zpětné srovnání zůstává na uživateli přes graf historie, ne jako dedikovaná funkce).
- Automatická upozornění/alerty při překročení bezpečnostního limitu větru.
- Zobrazení na sdílené obrazovce/infostánku (možné rozšíření, ne požadavek v1).

**Doplněno do rozsahu (best-effort odolnost proti výpadku spojení):**
- Pokud stanice ztratí spojení s backendem, ukládá naměřené hodnoty do vlastního lokálního logu a po obnovení spojení je zpětně doplní — best-effort, ne garance bezztrátovosti.
- Signalizace ztráty spojení přímo na stanici (např. LED), aby posádka na lodi věděla, že se aktuálně nezobrazují živá data.

## Rizika a otevřené otázky

- **Kalibrace magnetometru**: HMC5883L je citlivý na rušení kovem a elektronikou a nemá vestavěnou náklonovou kompenzaci — reálná přesnost směru se ověří až na místě. Riziko je nižší, než by bylo na kovové lodi: startovní loď je malá kajutová laminátová (nekovová) loď na kotvě, která se nehýbe, jen se může pootáčet. `[ASSUMPTION: nepřesnost směru v jednotkách stupňů je pro účel "bezpečné/nebezpečné" rozhodování akceptovatelná.]`
- **Dosah a spolehlivost Wi-Fi** na vodě (100–200 m k břehu nebo hotspot na lodi) není ověřený. Zmírněno v rozsahu v1 lokálním bufferováním a zpětným doplněním dat, ale míra ztrátovosti/zpoždění při delším výpadku se ověří až na místě.
- **Voděodolnost a mechanická instalace** (krabička, uchycení senzorů a magnetometru, stříkance, déšť) je součástí scope, ale technické řešení není v tomto briefu rozpracované — a s nejistou dostupností pomoci na elektromechanickou stránku (viz addendum) jde o riziko pro dodržení termínu, ne jen o detail.
- **Těsný termín** (~2 týdny do poloviny srpna) s jen jedním vývojářem na software i mechaniku — riziko, že se nestihne vše výše uvedené; pokud dojde na škrty, prioritou zůstává funkční aktuální čtení větru před historickým grafem a mechanickým provedením "na koleně".
- Konkrétní datum a místo závodu v polovině srpna 2026 nejsou v tomto briefu upřesněné. `[ASSUMPTION: upřesní se mimo brief, řešení k tomu není vázáno technicky.]`

## Vize

Pokud se stanice na prvním závodě osvědčí, přirozeným dalším krokem je rozšíření na víc stanic v rámci jednoho závodu (např. porovnání větru na startu vs. na trati) a jeden web, který by uměl zobrazit a archivovat data z více závodů napříč sezónami — z čehož by těžilo plánování budoucích ročníků i rozhodčí napříč oddíly, ne jen jeden tým.
