#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include "sense/anemometer.h"
#include "sense/vane.h"
#include "sense/magnetometer.h"
#include "correct/wind_speed.h"
#include "correct/wind_direction.h"
#include "transmit/reading.h"
#include "transmit/connection_monitor.h"
#include "transmit/buffer_capacity.h"
#include "transmit/led_policy.h"
#include "transmit/rssi_latch.h"
#include "transmit/hw/wifi_client.h"
#include "transmit/hw/clock.h"
#include "transmit/hw/flash_buffer.h"
#include "config/station_config.h"
#include "config/hw/config_loader.h"

namespace {
// Sampling interval per FR-1 (~2-5s) - exact value pending flash/buffer
// capacity check, see TODO.md and epics.md Story 1.1/2.1 open note.
constexpr unsigned long kSampleIntervalMs = 3000;
constexpr uint8_t kAnemometerPin = 27;
constexpr uint8_t kVanePin = 34;
constexpr uint8_t kDisconnectLedPin = 2;
// Diagnostic LEDs (2026-08-11, hardware bring-up): grouped with the sensor
// wiring above (27/34) on the EN/RST-button side of a standard 30-pin
// ESP32-WROOM DevKitC. Isolate WiFi-connect failures from backend-reachable
// failures, which kDisconnectLedPin alone can't distinguish - see
// docs/superpowers/specs/2026-08-11-firmware-diagnostics-design.md.
constexpr uint8_t kConfigLoadedLedPin = 25;  // on once config.txt parsed >=1 network
constexpr uint8_t kWifiConnectedLedPin = 26; // mirrors WiFi.status()==WL_CONNECTED
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
constexpr MagnetometerCalibration kMagnetometerCalibration{.hardIronOffsetX = 1713.5, .hardIronOffsetY = 1984.0};

Anemometer anemometer;
Vane vane;
Magnetometer magnetometer;
WifiTransmitClient transmitClient;
StationClock clock_;
ConnectionMonitor connectionMonitor;
FlashBuffer flashBuffer;
RssiAvailabilityLatch rssiLatch;
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
} // namespace

void setup() {
    Serial.begin(kSerialBaudRate);
    anemometer.begin(kAnemometerPin);
    vane.begin(kVanePin);
    bool magnetometerReady = magnetometer.begin();
    pinMode(kDisconnectLedPin, OUTPUT);
    pinMode(kConfigLoadedLedPin, OUTPUT);
    pinMode(kWifiConnectedLedPin, OUTPUT);
    // Story 4.1 (AD-10): credentials/token come from an on-flash config
    // file, never compiled into firmware source. An empty/missing config
    // (see config_loader.h) just means WiFi never connects - visible via
    // the disconnect LED - not a crash.
    StationConfig stationConfig = loadStationConfig();
    bool configLoaded = !stationConfig.networks.empty();
    digitalWrite(kConfigLoadedLedPin, configLoaded ? HIGH : LOW);
    Serial.println("\n--- Gustik station boot ---");
    Serial.printf("config.txt: %d network(s) configured, backend=%s\n",
                   static_cast<int>(stationConfig.networks.size()), stationConfig.backendBaseUrl.c_str());
    Serial.printf("magnetometer init: %s\n", magnetometerReady ? "ok" : "FAILED (I2C error - check wiring/address)");
    transmitClient.begin(stationConfig);
    clock_.begin();
    // NFR-4: buffer must cover >=4h at the sampling interval in use.
    flashBuffer.begin(computeBufferCapacityForHours(4.0, kSampleIntervalMs / 1000.0));
    lastSampleAt = millis();
}

void loop() {
    unsigned long now = millis();
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
        lastKnownYawDegrees = magnetometerHeadingDegrees(magX, magY, kMagnetometerCalibration);
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
    digitalWrite(kWifiConnectedLedPin, wifiConnectedThisCycle ? HIGH : LOW);

    // AC2: a failed send never blocks/crashes/restarts - it just leaves
    // this cycle's reading unsent and the loop continues on schedule.
    bool sent = transmitClient.send(&reading, 1);
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
            if (transmitClient.send(buffered.data(), buffered.size())) {
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

    Serial.printf("[%lums] wifi=%s%s%s%s%s%s%d sent=%s clockSynced=%s buffered=%u\n", now,
                  wifiConnectedThisCycle ? "up" : "down",
                  wifiConnectedThisCycle ? " ssid=" : "", wifiConnectedThisCycle ? WiFi.SSID().c_str() : "",
                  wifiConnectedThisCycle ? " ip=" : "", wifiConnectedThisCycle ? WiFi.localIP().toString().c_str() : "",
                  wifiConnectedThisCycle ? " rssi=" : "", wifiConnectedThisCycle ? WiFi.RSSI() : 0,
                  sent ? "yes" : "no", clock_.isSynced() ? "yes" : "no",
                  static_cast<unsigned>(flashBuffer.count()));
}
