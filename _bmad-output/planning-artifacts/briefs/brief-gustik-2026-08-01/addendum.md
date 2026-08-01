---
title: "Addendum: Gustik"
status: draft
created: 2026-08-01
updated: 2026-08-01
---

# Addendum: Gustik

Doplňkový materiál k `brief.md` — kontext, který se hodí pro navazující práci (plánování, architektura), ale nepatří do stručného briefu.

## Tým a role

- **Mlok** (uživatel/hlavní řešitel) — software, web/dashboard, programování firmwaru; primární plánovací předpoklad je jeden vývojář zvládající i mechanickou/instalační stránku.
- **Guru** (skautský bratr) — rád by se věnoval elektromechanické části (pájení, řešení umístění senzorů). Jeho zapojení je pravděpodobné, ale nejisté kvůli logistice.
- **Plánovací důsledek**: scope a termín (viz brief, sekce Rizika) počítají s tím, že vše — včetně mechaniky a instalace — může nakonec dělat jeden člověk. Pomoc od Gurua je bonus, který termín zkrátí nebo zlepší kvalitu mechanického provedení, ne předpoklad, na kterém stojí plán.

## Research digest (srovnatelná řešení a technické pozadí)

Zdroj: rychlý web research proveden během brainstormingu, pro zakotvení "proč stavět" argumentu a technického přístupu.

### 1. Srovnatelné projekty
Žádný nalezený projekt není postavený přesně pro použití na startovní/rozhodčí lodi s baterií+Wi-Fi+cloudem, ale existují silné stavební kameny:
- [curiouselectric/WindSensor](https://github.com/curiouselectric/WindSensor) — DIY rozhraní anemometr+korouhvička pro ESP32 logger, navržené pro nízkou spotřebu (sleep/wake). Nejbližší analogie k plánované architektuře.
- [pilotak/WeatherMeters](https://github.com/pilotak/WeatherMeters) — Arduino knihovna přímo pro senzory WH1080/WH1090/Sparkfun, interrupt-driven. Přímý precedent pro znovupoužití nakoupených senzorů.
- [taunusflieger/anemometer](https://github.com/taunusflieger/anemometer) (ESP32-S3, MQTT/AWS) a [alf45tar/Anemometer](https://github.com/alf45tar/Anemometer) (ESP32-C6, deep-sleep) — obecné nízkopříkonové cloud-connected uzly, dobrá reference pro bilanci spotřeby.
- [timhughes/esp32-yacht-wind-instrument](https://github.com/timhughes/esp32-yacht-wind-instrument) — nejbližší jachtařský precedent, ale je to zobrazovač pro existující NMEA/Signal K přístroje, ne logger senzor→cloud.
- [Hackaday: Seriously Upgraded Weather Station](https://hackaday.io/project/169445-seriously-upgraded-weather-station) — nahrazuje elektroniku WH1080 za ESP32, přímý precedent pro přístup "znovupoužití senzorů z hotové stanice".
- Nic nalezeného není postavené přímo pro bezpečnostní rozhodování rozhodčích na závodě — potvrzuje to mezeru na trhu, na které stojí "Co je jinak" v briefu.

### 2. Hotové alternativy (cenové zakotvení)
Ruční anemometry (Kestrel 1000/2000/3000) cca 80–160 $ (~75–150 €; podle [Practical Sailor](https://www.practical-sailor.com/marine-electronics/handheld-anemometers/)); levné varianty ~50 $ (~45 €). [Windie Pro 360](https://windie.pro/products/anemometer-pro-360) je cílený přímo na rozhodčí závodů. Žádný z nich nativně neumí kontinuální logování + živý webový dashboard + vícedenní historické srovnání — jsou to nástroje na jedno odečtení, ne na průběžné sledování. Žádný automaticky nekoriguje relativní směr větru na ukotvené, pootáčející se lodi.

### 3. Magnetometr pro korekci směru — funkční, ale je potřeba počítat s kalibrací
Použití magnetometrů typu HMC5883L pro heading je známý, běžný přístup, ale s reálnými úskalími: potřebuje hard-iron/soft-iron kalibraci (viz např. [diskuze na Arduino Fóru k wind vane + compass kalibraci](https://forum.arduino.cc/t/wind-vane-calibration-with-compass/377847)), degraduje poblíž kovu/elektroniky (na laminátové lodi nižší riziko než na kovové, ale motor/elektronika v blízkosti mohou stále rušit), nemá vestavěnou náklonovou kompenzaci (potřeba buď rovné umístění, nebo párování s akcelerometrem), a potřebuje lokální korekci magnetické deklinace. Běžné vylepšení: plná IMU fúze (akcelerometr+gyroskop+magnetometr, např. Madgwick/Mahony filtr) pro náklonově kompenzovaný heading, nebo GPS-course heading (přesný dual-antenna GNSS kompas je nadstandard/drahý; single-GPS course-over-ground funguje jen za pohybu — na kotvě k ničemu). Doporučení: počítat s časem na kalibraci; zvážit levný přídavný akcelerometr (řada breakout desek už ho má) pro náklonovou kompenzaci; GPS heading nechat jako možnou v2 cestu, ne požadavek v1.

### 4. WH1080/WH1090 — dobrý precedent
Dobře prošlapaná cesta: knihovna pilotak/WeatherMeters i Hackaday projekt přímo demonstrují čtení anemometru (reed-switch pulzy) a korouhvičky (odporový žebřík) z WH1080/WH1090 mikrokontrolérem, náhradou za původní konzoli. Počítání pulzů z reed-switche na ESP32 je přímočaré přes přerušení; čtení korouhvičky je jednoduché ADC čtení proti známým odporovým stupňům. Nízké technické riziko.
