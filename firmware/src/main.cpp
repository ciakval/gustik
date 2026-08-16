#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include "sense/anemometer.h"
#include "sense/vane.h"
#include "sense/vane_decode.h"
#include "sense/magnetometer.h"
#include "correct/wind_speed.h"
#include "correct/wind_direction.h"
#include "transmit/reading.h"
#include "transmit/connection_monitor.h"
#include "transmit/buffer_capacity.h"
#include "transmit/led_policy.h"
#include "transmit/rssi_latch.h"
#include "transmit/ingest_response.h"
#include "transmit/hw/wifi_client.h"
#include "transmit/hw/clock.h"
#include "transmit/hw/flash_buffer.h"
#include "config/station_config.h"
#include "config/hw/config_loader.h"

// Status LED panel (docs/superpowers/specs/2026-08-16-status-led-panel-design.md).
// Optional in three independent senses, any one of which is sufficient:
//   * build time - `-DGUSTIK_STATUS_PANEL=0` drops every reference below, the
//     linker's --gc-sections drops the code, and GPIO25/26 revert to the
//     `config loaded` / `WiFi connected` diagnostics they carried before;
//   * config time - `leds.enabled=false` in config.txt leaves the pins low
//     from boot and ignores the button, no reflash needed;
//   * wiring time - driving a GPIO with nothing attached is harmless and an
//     unwired INPUT_PULLUP button reads "released" forever, so a Station with
//     nothing soldered behaves exactly as it did before this change.
#ifndef GUSTIK_STATUS_PANEL
#define GUSTIK_STATUS_PANEL 1
#endif

#if GUSTIK_STATUS_PANEL
#include "indicate/button.h"
#include "indicate/panel.h"
#include "indicate/hw/button_pin.h"
#include "indicate/hw/led_panel.h"
#include "indicate/hw/panel_pins.h"
#endif

namespace {
// Sampling interval per FR-1 (~2-5s) - exact value pending flash/buffer
// capacity check, see TODO.md and epics.md Story 1.1/2.1 open note.
constexpr unsigned long kSampleIntervalMs = 3000;
constexpr uint8_t kAnemometerPin = 27;
constexpr uint8_t kVanePin = 34;
// Story 2.4 / FR-5, the onboard LED. Unchanged by the status panel, so the
// "signal a lost connection" requirement is still satisfied with the panel
// absent, disabled or asleep.
constexpr uint8_t kDisconnectLedPin = 2;
#if !GUSTIK_STATUS_PANEL
// Diagnostic LEDs (2026-08-11, hardware bring-up): grouped with the sensor
// wiring above (27/34) on the EN/RST-button side of the board. Isolate
// WiFi-connect failures from backend-reachable failures, which
// kDisconnectLedPin alone can't distinguish - see
// docs/superpowers/specs/2026-08-11-firmware-diagnostics-design.md. The
// status panel takes both pins over and covers both meanings (its GREEN and
// YELLOW status lanes), so these only exist when it is compiled out.
constexpr uint8_t kConfigLoadedLedPin = 25;  // on once config.txt parsed >=1 network
constexpr uint8_t kWifiConnectedLedPin = 26; // mirrors WiFi.status()==WL_CONNECTED
#endif
constexpr unsigned long kSerialBaudRate = 115200;

// TODO(calibration): measure against a reference anemometer before
// deployment - see correct/wind_speed.h.
constexpr AnemometerCalibration kAnemometerCalibration{.metersPerSecondPerHz = 1.2};

// Hard-iron offsets measured 2026-08-11 on the bench (scripts/, tumble
// calibration - see scripts/qmc5883p-calibration.json: offset=[1713.5,
// -1984.0, 1714.0] at field_range=8G, which sense/magnetometer.cpp's
// begin() also sets). hardIronOffsetY is the *negative* of the JSON's raw
// Y offset (-1984.0 -> 1984.0) because magnetometer.cpp's readRawXY()
// returns Y already sign-flipped for the confirmed real mount orientation
// (up=-z, forward=+x) - see the comment there. Soft-iron (scale) is not
// applied - deliberately deferred, see wind_direction.h. NOTE: this
// calibration describes one rigid assembly (scripts/README.md) - it was
// captured on the bench, not the final boat-mounted enclosure; redo it if
// the sensor's mount or magnetic surroundings change before Story 5.2.
// config.txt's `mag.offsetX`/`mag.offsetY` override this at boot when both
// are present (see config/station_config.h), so a recalibration after
// remounting the sensor is a filesystem upload rather than a reflash. The
// compiled-in value below is the fallback for a config that says nothing
// about the magnetometer.
constexpr MagnetometerCalibration kDefaultMagnetometerCalibration{.hardIronOffsetX = 1713.5, .hardIronOffsetY = 1984.0};
MagnetometerCalibration magnetometerCalibration = kDefaultMagnetometerCalibration;

Anemometer anemometer;
Vane vane;
Magnetometer magnetometer;
WifiTransmitClient transmitClient;
StationClock clock_;
ConnectionMonitor connectionMonitor;
FlashBuffer flashBuffer;
RssiAvailabilityLatch rssiLatch;
// File scope (was a setup() local) so the panel can name which of the
// configured networks is actually associated - "you are on somebody's phone
// hotspot" is one of the questions the panel exists to answer.
StationConfig stationConfig;
bool stationConfigLoaded = false;
unsigned long lastSampleAt = 0;
unsigned long nextClientSeq = 0;
// Falls back to the last successfully-read heading on a magnetometer I2C
// failure (see sense/magnetometer.h) instead of dropping the reading or
// blocking - defaults to 0deg/north until the first successful read.
double lastKnownYawDegrees = 0.0;

// clientId must stay unique across reboots, not just within one boot - the
// backend's client_id UNIQUE constraint (ON CONFLICT DO NOTHING) is what
// makes backfill retries safe (Story 2.2), but it silently no-ops any
// POST whose clientId collides with a row from an earlier boot, and
// POST /readings still returns 201 either way (see bug-031). A counter
// that resets to 0 on every reboot collides with low-numbered clientIds
// this same device (same WiFi.macAddress()) already sent and stored in a
// previous boot - exactly what happened during real-hardware testing.
// Folding in the reading's own capturedAt (wall-clock, not boot-relative)
// makes collisions require the same device to reboot within the same
// captured second, which the sample interval (>=3s apart) and reboot time
// both rule out in practice; nextClientSeq stays only as a same-boot
// tie-breaker in case the clock never syncs.
String makeClientId(const std::string &capturedAt) {
    return WiFi.macAddress() + "-" + String(capturedAt.c_str()) + "-" + String(nextClientSeq++);
}

#if GUSTIK_STATUS_PANEL
StatusPanel statusPanel;
LedPanel ledPanel;
ButtonPin panelButton;
ButtonDecoder buttonDecoder;
// Refreshed wholesale at the end of every sample cycle; only the anemometer
// snapshot is re-read on every tick, so the live reed-closure indicator can
// flash faster than the 3 s sampling interval.
PanelInputs panelInputs;

// Which configured network we are actually on (index into
// stationConfig.networks), or -1 if unknown. Derived from WiFi.SSID()
// rather than plumbed out of WifiTransmitClient, which keeps the transmit
// path unaware that the panel exists - design constraint C3.
int connectedNetworkIndex() {
    if (WiFi.status() != WL_CONNECTED) {
        return -1;
    }
    String ssid = WiFi.SSID();
    for (size_t i = 0; i < stationConfig.networks.size(); i++) {
        if (ssid == stationConfig.networks[i].ssid.c_str()) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// Runs on every loop() iteration, before the sampling gate - the panel needs
// sub-second resolution for blinking, and loop() early-returns between
// samples. Cheap, non-blocking, allocation-free: nine digitalWrites and a
// digitalRead over a state machine that never touches anything upstream.
void panelTick(unsigned long now) {
    panelInputs.pulseCountSnapshot = anemometer.pulseCountSnapshot();
    ButtonEvent event = buttonDecoder.update(panelButton.isPressed(), now);
    PanelOutputs outputs = statusPanel.tick(panelInputs, now, event);
    ledPanel.render(outputs, now);
}
#endif
} // namespace

void setup() {
    Serial.begin(kSerialBaudRate);
    anemometer.begin(kAnemometerPin);
    vane.begin(kVanePin);
    bool magnetometerReady = magnetometer.begin();
    pinMode(kDisconnectLedPin, OUTPUT);
#if !GUSTIK_STATUS_PANEL
    pinMode(kConfigLoadedLedPin, OUTPUT);
    pinMode(kWifiConnectedLedPin, OUTPUT);
#endif
    // Story 4.1 (AD-10): credentials/token come from an on-flash config
    // file, never compiled into firmware source. An empty/missing config
    // (see config_loader.h) just means WiFi never connects - visible via
    // the disconnect LED - not a crash.
    stationConfig = loadStationConfig();
    stationConfigLoaded = !stationConfig.networks.empty();
#if !GUSTIK_STATUS_PANEL
    digitalWrite(kConfigLoadedLedPin, stationConfigLoaded ? HIGH : LOW);
#endif
    Serial.println("\n--- Gustik station boot ---");
    Serial.printf("config.txt: %d network(s) configured, backend=%s\n",
                   static_cast<int>(stationConfig.networks.size()), stationConfig.backendBaseUrl.c_str());
    Serial.printf("magnetometer init: %s\n", magnetometerReady ? "ok" : "FAILED (I2C error - check wiring/address)");
    if (stationConfig.magnetometer.present) {
        magnetometerCalibration.hardIronOffsetX = stationConfig.magnetometer.offsetX;
        magnetometerCalibration.hardIronOffsetY = stationConfig.magnetometer.offsetY;
    }
    // Print it either way: a wrong hard-iron offset produces a confident,
    // stable, wrong heading with no error anywhere, so which numbers are
    // actually in force has to be visible at boot.
    Serial.printf("magnetometer hard-iron: x=%.1f y=%.1f (%s)\n",
                  magnetometerCalibration.hardIronOffsetX, magnetometerCalibration.hardIronOffsetY,
                  stationConfig.magnetometer.present ? "from config.txt" : "compiled-in default");
#if GUSTIK_STATUS_PANEL
    LedPanelPins panelPins;
    panelPins.status[kStatusRed] = GUSTIK_PANEL_PIN_STATUS_RED;
    panelPins.status[kStatusYellow] = GUSTIK_PANEL_PIN_STATUS_YELLOW;
    panelPins.status[kStatusGreen] = GUSTIK_PANEL_PIN_STATUS_GREEN;
    panelPins.status[kStatusBlue] = GUSTIK_PANEL_PIN_STATUS_BLUE;
    panelPins.detail[0] = GUSTIK_PANEL_PIN_DETAIL_1;
    panelPins.detail[1] = GUSTIK_PANEL_PIN_DETAIL_2;
    panelPins.detail[2] = GUSTIK_PANEL_PIN_DETAIL_3;
    panelPins.detail[3] = GUSTIK_PANEL_PIN_DETAIL_4;
    panelPins.detail[4] = GUSTIK_PANEL_PIN_DETAIL_5;
    ledPanel.begin(panelPins);
    panelButton.begin(GUSTIK_PANEL_PIN_BUTTON);
    PanelSettings panelSettings;
    panelSettings.enabled = stationConfig.leds.enabled;
    panelSettings.sleepTimeoutMs = stationConfig.leds.timeoutSeconds * 1000UL;
    statusPanel.begin(panelSettings, millis());
    panelInputs.configLoaded = stationConfigLoaded;
    Serial.printf("status LED panel: %s, sleep after %lus\n",
                  panelSettings.enabled ? "enabled" : "disabled (leds.enabled=false)",
                  stationConfig.leds.timeoutSeconds);
#endif

    transmitClient.begin(stationConfig);
    clock_.begin();
    // NFR-4: buffer must cover >=4h at the sampling interval in use.
    flashBuffer.begin(computeBufferCapacityForHours(4.0, kSampleIntervalMs / 1000.0));
    lastSampleAt = millis();
}

void loop() {
    unsigned long now = millis();
#if GUSTIK_STATUS_PANEL
    // Before the sampling gate below, which early-returns for ~3 s at a time
    // and would otherwise make any sub-second blinking impossible.
    panelTick(now);
#endif
    if (now - lastSampleAt < kSampleIntervalMs) {
        return;
    }
    double intervalSeconds = (now - lastSampleAt) / 1000.0;
    lastSampleAt = now;

    unsigned long pulses = anemometer.readAndResetPulseCount();
    double windSpeedMs = pulsesToWindSpeedMs(pulses, intervalSeconds, kAnemometerCalibration);

    int rawVaneOctant = vane.readRawOctant();
    double magX = 0.0, magY = 0.0;
    bool magnetometerOk = magnetometer.readRawXY(magX, magY);
    if (magnetometerOk) {
        lastKnownYawDegrees = magnetometerHeadingDegrees(magX, magY, magnetometerCalibration);
    }
    // A failed I2C read never blocks this cycle or drops the reading - it
    // just reuses the last successfully-measured heading (see bug-030).
    double yawDegrees = lastKnownYawDegrees;
    int windDirOctant = correctWindDirectionOctant(rawVaneOctant, yawDegrees);

    Serial.printf("[%lums] sensors: pulses=%lu windSpeedMs=%.2f vaneOctant=%d magnetometer=%s yawDeg=%.1f windDirOctant=%d\n", now,
                  pulses, windSpeedMs, rawVaneOctant,
                  magnetometerOk ? "ok" : "FAIL(using last-known heading)",
                  yawDegrees, windDirOctant);

    Reading reading;
    reading.capturedAt = clock_.nowIso8601().c_str();
    reading.clientId = makeClientId(reading.capturedAt).c_str();
    reading.clockSynced = clock_.isSynced();
    reading.windSpeedMs = windSpeedMs;
    reading.windDirOctant = windDirOctant;
    // Story 2.5 (FR-6): standard ESP32 WiFi API, read at the moment of
    // measurement. Latch stays true through brief reconnects once the
    // Station has scanned successfully at least once since boot (AC2).
    bool wifiConnectedThisCycle = WiFi.status() == WL_CONNECTED;
    rssiLatch.recordScanResult(wifiConnectedThisCycle);
    reading.rssiValid = rssiLatch.isAvailable();
    reading.rssiDbm = reading.rssiValid ? WiFi.RSSI() : 0;
#if !GUSTIK_STATUS_PANEL
    digitalWrite(kWifiConnectedLedPin, wifiConnectedThisCycle ? HIGH : LOW);
#endif

    // AC2: a failed send never blocks/crashes/restarts - it just leaves
    // this cycle's reading unsent and the loop continues on schedule.
    SendResult sendResult = transmitClient.send(&reading, 1);
    bool sent = sendResult.ok;
    if (sent) {
        connectionMonitor.recordSendSuccess();
        // Story 2.2 AC1: on the reconnect that ends an outage, send
        // everything buffered during it as one array-of-N POST (AD-8),
        // oldest-to-newest (AD-2). Only clear the buffer once that backfill
        // send actually succeeds (AC2) - a failure leaves it queued for the
        // next reconnect, and already-received records are safe from
        // duplication via the backend's client_id UNIQUE constraint either way.
        if (connectionMonitor.justRecovered() && flashBuffer.count() > 0) {
            std::vector<Reading> buffered = flashBuffer.peekAll();
            SendResult backfillResult = transmitClient.send(buffered.data(), buffered.size());
            Serial.printf("[%lums] backfill: %u buffered reading(s) %s\n", now,
                          static_cast<unsigned>(buffered.size()),
                          describeIngestOutcome(backfillResult.ok, backfillResult.statusCode,
                                                backfillResult.response)
                              .c_str());
            if (backfillResult.ok) {
                flashBuffer.clear();
            }
        }
    } else {
        // Story 2.1: buffer locally instead of dropping - best-effort
        // (FR-4), sampling keeps going regardless of buffer/flash state.
        connectionMonitor.recordSendFailure();
        flashBuffer.push(reading);
    }
    // Story 2.4 (FR-5/NFR-2): reflects connection health on every cycle,
    // immediately - see led_policy.h for why no debounce is used here.
    digitalWrite(kDisconnectLedPin, shouldLedSignalDisconnect(connectionMonitor.isHealthy()) ? HIGH : LOW);
    clock_.resyncIfNeeded();

    // `sent=yes` alone was actively misleading: it meant "the POST returned
    // 2xx", not "the backend stored the reading" - the exact gap bug-031
    // hid in. describeIngestOutcome() reports the backend's own
    // stored/received counts and shouts when a successful request stored
    // nothing at all.
    Serial.printf("[%lums] wifi=%s%s%s%s%s%s%d %s clockSynced=%s buffered=%u\n", now,
                  wifiConnectedThisCycle ? "up" : "down",
                  wifiConnectedThisCycle ? " ssid=" : "", wifiConnectedThisCycle ? WiFi.SSID().c_str() : "",
                  wifiConnectedThisCycle ? " ip=" : "", wifiConnectedThisCycle ? WiFi.localIP().toString().c_str() : "",
                  wifiConnectedThisCycle ? " rssi=" : "", wifiConnectedThisCycle ? WiFi.RSSI() : 0,
                  describeIngestOutcome(sendResult.ok, sendResult.statusCode, sendResult.response).c_str(),
                  clock_.isSynced() ? "yes" : "no",
                  static_cast<unsigned>(flashBuffer.count()));

#if GUSTIK_STATUS_PANEL
    // The panel's whole view of the world, refreshed once per cycle from
    // values this function has already computed for its own diagnostics.
    // Strictly one-way (design constraint C3): nothing below this line may
    // ever write back into a sensor, the buffer or the transmit client.
    panelInputs.lastSampleAtMs = now;
    panelInputs.haveSample = true;
    panelInputs.configLoaded = stationConfigLoaded;
    panelInputs.wifiAssociated = wifiConnectedThisCycle;
    panelInputs.rssiValid = reading.rssiValid;
    panelInputs.rssiDbm = reading.rssiDbm;
    panelInputs.networkIndex = connectedNetworkIndex();
    panelInputs.lastSendOk = sendResult.ok;
    panelInputs.lastHttpStatus = sendResult.statusCode;
    panelInputs.hasCounts = sendResult.response.hasCounts;
    panelInputs.lastInserted = sendResult.response.inserted;
    panelInputs.bufferedCount = flashBuffer.count();
    panelInputs.clockSynced = clock_.isSynced();
    panelInputs.magnetometerOk = magnetometerOk;
    // vaneAdcPlausible() reads the raw ADC behind readRawOctant() above -
    // the octant alone cannot report a broken vane, since the decode is
    // total by design and an open pin resolves to a confident 270deg.
    panelInputs.vaneInRange = vaneAdcPlausible(vane.lastRawAdc());
    panelInputs.windSpeedMs = windSpeedMs;
    panelInputs.windDirOctant = windDirOctant;
#endif
}
