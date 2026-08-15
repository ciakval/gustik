// Wind-vane bring-up diagnostic - standalone firmware, Serial output only.
//
// Purpose: prove that the salvaged WH1080/WH1090 wind vane is wired and
// behaving correctly BEFORE trusting sense/vane.cpp's placeholder
// `kOctantAdcReadings` table (see TODO.md - those 8 numbers were never
// measured on real hardware). This build talks to nothing else: no WiFi, no
// backend, no magnetometer, no anemometer, no LittleFS config. Flash it,
// open the serial monitor, slowly rotate the vane through a full turn, and
// read the output.
//
// Build/flash (this env only builds this file - main.cpp is excluded):
//   pio run -e vane_diag -t upload && pio device monitor -e vane_diag
//
// Wiring it assumes (docs/hardware/wind-sensor-wiring.md):
//   3.3V --[10k]-- GPIO34 --(RJ11 pin 1)--[vane]--(RJ11 pin 4)-- GND
// GPIO34 is input-only with NO internal pull resistor, so the external 10k
// is mandatory - a missing pull-up is one of the failure modes this sketch
// is written to name out loud rather than leave to be guessed at.
//
// What the output is for: every line is key=value so a whole capture can be
// analysed after the fact. The per-sample lines show what the ADC sees right
// now; the periodic SUMMARY block is the actual verdict - it clusters the
// stable levels seen so far, matches each against the manufacturer's
// resistance table, and prints a ready-to-paste replacement for
// `kOctantAdcReadings`.

#include <Arduino.h>

namespace {

// ---------------------------------------------------------------------------
// Wiring constants - the two numbers to change if the bench rig differs.
// ---------------------------------------------------------------------------

constexpr uint8_t kVanePin = 34;          // must match main.cpp's kVanePin
constexpr double kPullUpOhms = 10000.0;   // external pull-up to 3.3V
constexpr double kSupplyMv = 3300.0;      // ESP32 3.3V rail, nominal

constexpr unsigned long kSerialBaudRate = 115200;
constexpr unsigned long kSamplePeriodMs = 250;
constexpr unsigned long kSummaryPeriodMs = 15000;

// ---------------------------------------------------------------------------
// Reference table - Fine Offset's own datasheet resistances for the vane's
// 16 positions (docs/hardware/wind-sensor-wiring.md). The firmware proper
// only uses the 8 primary octants, but decoding against all 16 here means a
// vane parked between two reed switches reads as "112.5deg" instead of
// silently mis-matching one of its neighbours.
// ---------------------------------------------------------------------------

struct ReferencePosition {
    double degrees;
    double ohms;
    const char *czechAbbrev; // S=sever/north, J=jih/south, V=vychod, Z=zapad
};

constexpr ReferencePosition kReference[] = {
    {0.0, 33000.0, "S"},      {22.5, 6570.0, "SSV"},   {45.0, 8200.0, "SV"},
    {67.5, 891.0, "VSV"},     {90.0, 1000.0, "V"},     {112.5, 688.0, "VJV"},
    {135.0, 2200.0, "JV"},    {157.5, 1410.0, "JJV"},  {180.0, 3900.0, "J"},
    {202.5, 3140.0, "JJZ"},   {225.0, 16000.0, "JZ"},  {247.5, 14120.0, "ZJZ"},
    {270.0, 120000.0, "Z"},   {292.5, 42120.0, "ZSZ"}, {315.0, 64900.0, "SZ"},
    {337.5, 21880.0, "SSZ"},
};
constexpr int kReferenceCount = sizeof(kReference) / sizeof(kReference[0]);

// ---------------------------------------------------------------------------
// Thresholds. All in millivolts at the pin.
// ---------------------------------------------------------------------------

// A burst whose min/max span more than this is mid-transition (the vane is
// being turned, or a reed switch is bouncing) - shown live, but not allowed
// to create a cluster.
constexpr int kStableSpreadMv = 40;
// Consecutive agreeing stable bursts required before a level is recorded.
constexpr int kStableRunRequired = 4; // ~1s settled
// Two readings within this are treated as the same physical vane position.
constexpr int kClusterToleranceMv = 60;
// Above this the ESP32's 11dB ADC is close to saturation and the derived
// resistance stops being trustworthy (120k reads ~3046mV by design).
constexpr int kAdcCeilingWarnMv = 2900;
// Distance from the nearest reference position beyond which a level is
// called unexplained rather than matched.
constexpr int kMatchWarnMv = 120;

constexpr int kSamplesPerBurst = 16;
constexpr int kMaxClusters = 24;

// ---------------------------------------------------------------------------

struct Cluster {
    double sumMv;
    double sumAdc;
    uint32_t count;
    int minMv;
    int maxMv;
};

Cluster clusters[kMaxClusters];
int clusterCount = 0;

double expectedMv[kReferenceCount];

unsigned long lastSampleMs = 0;
unsigned long lastSummaryMs = 0;
unsigned long startedMs = 0;

int lastStableMv = -10000;
int stableRun = 0;

// Divider maths. The vane sits between the pin and GND, so a LOW voltage
// means a LOW resistance (90deg = 1k reads ~300mV; 270deg = 120k reads
// ~3046mV).
double expectedMvForOhms(double ohms) {
    return kSupplyMv * ohms / (ohms + kPullUpOhms);
}

// Inverse of the above. Returns -1 when the reading is at/over the rail,
// where the division blows up and the answer would be meaningless anyway.
double ohmsForMv(double mv) {
    if (mv >= kSupplyMv - 1.0) {
        return -1.0;
    }
    return kPullUpOhms * mv / (kSupplyMv - mv);
}

struct Match {
    int index;      // index into kReference
    int errorMv;    // signed: measured - expected
    int marginMv;   // distance to the *second* best match; small = ambiguous
};

Match matchReference(double mv) {
    Match result{0, 0, 0};
    double best = 1e9;
    double secondBest = 1e9;
    for (int i = 0; i < kReferenceCount; i++) {
        double distance = fabs(mv - expectedMv[i]);
        if (distance < best) {
            secondBest = best;
            best = distance;
            result.index = i;
        } else if (distance < secondBest) {
            secondBest = distance;
        }
    }
    result.errorMv = static_cast<int>(lround(mv - expectedMv[result.index]));
    result.marginMv = static_cast<int>(lround(secondBest - best));
    return result;
}

void resetClusters() {
    clusterCount = 0;
    lastStableMv = -10000;
    stableRun = 0;
    startedMs = millis();
}

void recordLevel(int mv, int adc) {
    for (int i = 0; i < clusterCount; i++) {
        double mean = clusters[i].sumMv / clusters[i].count;
        if (fabs(mv - mean) <= kClusterToleranceMv) {
            clusters[i].sumMv += mv;
            clusters[i].sumAdc += adc;
            clusters[i].count++;
            if (mv < clusters[i].minMv) clusters[i].minMv = mv;
            if (mv > clusters[i].maxMv) clusters[i].maxMv = mv;
            return;
        }
    }
    if (clusterCount >= kMaxClusters) {
        return; // 24 distinct levels already means something is badly wrong
    }
    clusters[clusterCount] = Cluster{static_cast<double>(mv), static_cast<double>(adc), 1, mv, mv};
    clusterCount++;
}

// Insertion sort - kSamplesPerBurst is 16, and pulling in <algorithm> for
// this would be the only STL dependency in the file.
void sortAscending(int *values, int count) {
    for (int i = 1; i < count; i++) {
        int key = values[i];
        int j = i - 1;
        while (j >= 0 && values[j] > key) {
            values[j + 1] = values[j];
            j--;
        }
        values[j + 1] = key;
    }
}

void printBanner() {
    Serial.println();
    Serial.println(F("=========================================================="));
    Serial.println(F("  Gustik - wind vane direction diagnostic"));
    Serial.println(F("=========================================================="));
    Serial.printf("pin=GPIO%u  pullUp=%.0fohm  supply=%.0fmV  adcBits=12  atten=11dB\n",
                  kVanePin, kPullUpOhms, kSupplyMv);
    Serial.println(F("wiring assumed: 3.3V --[10k]-- GPIO34 --[vane]-- GND"));
    Serial.println(F("(GPIO34 has no internal pull-up - the external one is mandatory)"));
    Serial.println();
    Serial.println(F("expected level for each of the vane's 16 reed positions:"));
    Serial.println(F("  deg    dir      ohms      mV     adc"));
    for (int i = 0; i < kReferenceCount; i++) {
        double mv = expectedMv[i];
        Serial.printf("  %5.1f  %-4s  %8.0f  %6.0f  %6.0f\n",
                      kReference[i].degrees, kReference[i].czechAbbrev,
                      kReference[i].ohms, mv, mv / kSupplyMv * 4095.0);
    }
    Serial.println();
    Serial.println(F("TURN THE VANE SLOWLY THROUGH A FULL 360deg, pausing ~2s at each"));
    Serial.println(F("detent. A SUMMARY block prints every 15s with the verdict."));
    Serial.println(F("Serial commands:  s = summary now   r = reset collected levels"));
    Serial.println(F("=========================================================="));
    Serial.println();
}

void printSummary() {
    unsigned long elapsedS = (millis() - startedMs) / 1000;
    Serial.println();
    Serial.printf("=== SUMMARY t=%lus levels=%d ===\n", elapsedS, clusterCount);

    if (clusterCount == 0) {
        Serial.println(F("  no stable level recorded yet - is the vane connected and still?"));
        Serial.println(F("=== END SUMMARY ==="));
        Serial.println();
        return;
    }

    // Sort cluster indices by mean voltage so the table reads bottom-up.
    int order[kMaxClusters];
    for (int i = 0; i < clusterCount; i++) order[i] = i;
    for (int i = 1; i < clusterCount; i++) {
        int key = order[i];
        double keyMean = clusters[key].sumMv / clusters[key].count;
        int j = i - 1;
        while (j >= 0 && (clusters[order[j]].sumMv / clusters[order[j]].count) > keyMean) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = key;
    }

    Serial.println(F("   #    mV   spread     adc       ohms   nearest        err  margin"));

    // Which primary octant (0,45,...315 -> reference index 0,2,4,...14) each
    // cluster claims, for the coverage checklist and the paste-ready table.
    int octantCluster[8];
    double octantAdc[8];
    for (int i = 0; i < 8; i++) {
        octantCluster[i] = -1;
        octantAdc[i] = 0.0;
    }
    bool sawAmbiguous = false;
    bool sawUnexplained = false;
    bool sawCeiling = false;
    bool sawCollision = false;

    for (int n = 0; n < clusterCount; n++) {
        const Cluster &c = clusters[order[n]];
        double meanMv = c.sumMv / c.count;
        double meanAdc = c.sumAdc / c.count;
        double ohms = ohmsForMv(meanMv);
        Match m = matchReference(meanMv);

        char ohmsText[12];
        if (ohms < 0.0) {
            snprintf(ohmsText, sizeof(ohmsText), "%10s", "open");
        } else {
            snprintf(ohmsText, sizeof(ohmsText), "%10.0f", ohms);
        }

        Serial.printf("  %2d  %5.0f   %6d  %6.0f %s   %5.1f %-4s  %+5d  %5d",
                      n + 1, meanMv, c.maxMv - c.minMv, meanAdc, ohmsText,
                      kReference[m.index].degrees, kReference[m.index].czechAbbrev,
                      m.errorMv, m.marginMv);

        if (abs(m.errorMv) > kMatchWarnMv) {
            Serial.print(F("  <- UNEXPLAINED"));
            sawUnexplained = true;
        }
        if (m.marginMv < kClusterToleranceMv) {
            Serial.print(F("  <- AMBIGUOUS"));
            sawAmbiguous = true;
        }
        if (meanMv > kAdcCeilingWarnMv) {
            Serial.print(F("  <- NEAR ADC CEILING"));
            sawCeiling = true;
        }
        Serial.printf("  n=%lu\n", static_cast<unsigned long>(c.count));

        // Even-indexed references are the 8 primary octants.
        if (m.index % 2 == 0 && abs(m.errorMv) <= kMatchWarnMv) {
            int octant = m.index / 2;
            if (octantCluster[octant] >= 0) {
                sawCollision = true;
            }
            octantCluster[octant] = order[n];
            octantAdc[octant] = meanAdc;
        }
    }

    // Coverage checklist - tells whoever is turning the vane what is left.
    Serial.print(F("  octants seen:"));
    int octantsSeen = 0;
    for (int i = 0; i < 8; i++) {
        const ReferencePosition &ref = kReference[i * 2];
        if (octantCluster[i] >= 0) {
            octantsSeen++;
            Serial.printf(" [%.0f %s]", ref.degrees, ref.czechAbbrev);
        } else {
            Serial.printf(" (%.0f %s?)", ref.degrees, ref.czechAbbrev);
        }
    }
    Serial.printf("  -> %d/8\n", octantsSeen);

    // Verdict.
    if (octantsSeen == 8 && !sawUnexplained && !sawAmbiguous && !sawCollision) {
        Serial.println(F("  VERDICT: all 8 octants seen, each matches a distinct datasheet"));
        Serial.println(F("           position - vane looks correctly wired and readable."));
    } else {
        Serial.println(F("  VERDICT: not clean yet -"));
        if (octantsSeen < 8) {
            Serial.println(F("    - keep turning; some octants have not been visited"));
        }
        if (sawCollision) {
            Serial.println(F("    - two different levels decoded to the SAME octant: the"));
            Serial.println(F("      pull-up value or supply constant above is likely wrong"));
        }
        if (sawUnexplained) {
            Serial.println(F("    - a level matches no datasheet position: wrong pull-up"));
            Serial.println(F("      value, added cable resistance, or not the WH1080 vane"));
        }
        if (sawAmbiguous) {
            Serial.println(F("    - two datasheet positions sit within noise of each other"));
            Serial.println(F("      at this pull-up value; 8.2k/6.57k and 1k/891 are the"));
            Serial.println(F("      usual pair, mostly harmless for 8-octant use"));
        }
        if (sawCeiling) {
            Serial.println(F("    - a level is near the ADC ceiling (270deg=120k reads"));
            Serial.println(F("      ~3046mV by design). If 270deg and 315deg collapse into"));
            Serial.println(F("      one level, drop the pull-up to 4.7k and re-run"));
        }
    }

    // Paste-ready calibration table for sense/vane.cpp.
    if (octantsSeen > 0) {
        Serial.println(F("  measured kOctantAdcReadings[8] for firmware/src/sense/vane.cpp:"));
        Serial.print(F("    {"));
        for (int i = 0; i < 8; i++) {
            if (octantCluster[i] >= 0) {
                Serial.printf("%d", static_cast<int>(lround(octantAdc[i])));
            } else {
                Serial.print(F("????"));
            }
            if (i < 7) Serial.print(F(", "));
        }
        Serial.println(F("},"));
        if (octantsSeen < 8) {
            Serial.println(F("    (???? = octant not visited yet - not ready to paste)"));
        }
    }

    Serial.println(F("=== END SUMMARY ==="));
    Serial.println();
}

void handleSerialCommand() {
    while (Serial.available() > 0) {
        int c = Serial.read();
        if (c == 's' || c == 'S') {
            printSummary();
            lastSummaryMs = millis();
        } else if (c == 'r' || c == 'R') {
            resetClusters();
            Serial.println(F("-- collected levels reset --"));
        }
    }
}

} // namespace

void setup() {
    Serial.begin(kSerialBaudRate);
    delay(300); // let the USB-serial bridge settle before the banner

    analogReadResolution(12);
    analogSetPinAttenuation(kVanePin, ADC_11db); // full 0-3.3V range
    pinMode(kVanePin, INPUT);                    // input-only pin; no pull available

    for (int i = 0; i < kReferenceCount; i++) {
        expectedMv[i] = expectedMvForOhms(kReference[i].ohms);
    }

    resetClusters();
    printBanner();
    lastSummaryMs = millis();
}

void loop() {
    handleSerialCommand();

    unsigned long now = millis();
    if (now - lastSampleMs < kSamplePeriodMs) {
        return;
    }
    lastSampleMs = now;

    int millivolts[kSamplesPerBurst];
    int counts[kSamplesPerBurst];
    for (int i = 0; i < kSamplesPerBurst; i++) {
        // analogReadMilliVolts applies the chip's own eFuse ADC calibration
        // curve; the raw count is printed alongside it because that is what
        // vane.cpp actually compares against.
        millivolts[i] = analogReadMilliVolts(kVanePin);
        counts[i] = analogRead(kVanePin);
        delayMicroseconds(200);
    }
    sortAscending(millivolts, kSamplesPerBurst);
    sortAscending(counts, kSamplesPerBurst);

    int medianMv = millivolts[kSamplesPerBurst / 2];
    int medianAdc = counts[kSamplesPerBurst / 2];
    int spreadMv = millivolts[kSamplesPerBurst - 1] - millivolts[0];

    // Hard failure modes get named rather than decoded.
    if (medianAdc >= 4090) {
        Serial.printf("[%8lu] mv=%d adc=%d !! PIN AT CEILING - open circuit: vane "
                      "unplugged, broken wire, or pull-up on the wrong pin\n",
                      now, medianMv, medianAdc);
        return;
    }
    if (medianAdc <= 5) {
        Serial.printf("[%8lu] mv=%d adc=%d !! PIN AT GROUND - no pull-up fitted, "
                      "or the vane pin is shorted to GND\n",
                      now, medianMv, medianAdc);
        return;
    }

    bool stable = spreadMv <= kStableSpreadMv;
    if (stable && abs(medianMv - lastStableMv) <= kClusterToleranceMv) {
        stableRun++;
    } else {
        stableRun = stable ? 1 : 0;
    }
    if (stable) {
        lastStableMv = medianMv;
    }
    if (stable && stableRun >= kStableRunRequired) {
        recordLevel(medianMv, medianAdc);
    }

    double ohms = ohmsForMv(medianMv);
    Match m = matchReference(medianMv);

    Serial.printf("[%8lu] mv=%4d adc=%4d spread=%3d ohms=", now, medianMv, medianAdc, spreadMv);
    if (ohms < 0.0) {
        Serial.print(F("  open"));
    } else {
        Serial.printf("%6.0f", ohms);
    }
    Serial.printf(" dir=%5.1fdeg %-4s err=%+5dmV margin=%4dmV %s\n",
                  kReference[m.index].degrees, kReference[m.index].czechAbbrev,
                  m.errorMv, m.marginMv,
                  stable ? (stableRun >= kStableRunRequired ? "settled" : "settling")
                         : "TURNING");

    if (now - lastSummaryMs >= kSummaryPeriodMs) {
        lastSummaryMs = now;
        printSummary();
    }
}
