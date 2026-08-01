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
#include "transmit/hw/wifi_client.h"
#include "transmit/hw/clock.h"
#include "transmit/hw/flash_buffer.h"

namespace {
// Sampling interval per FR-1 (~2-5s) - exact value pending flash/buffer
// capacity check, see TODO.md and epics.md Story 1.1/2.1 open note.
constexpr unsigned long kSampleIntervalMs = 3000;
constexpr uint8_t kAnemometerPin = 27;
constexpr uint8_t kVanePin = 34;
constexpr uint8_t kDisconnectLedPin = 2;

// TODO(calibration): measure against a reference anemometer before
// deployment - see correct/wind_speed.h.
constexpr AnemometerCalibration kAnemometerCalibration{.metersPerSecondPerHz = 1.2};

// TODO(calibration): hard-iron calibration for the actual installed
// magnetometer/boat mount - see correct/wind_direction.h and TODO.md.
constexpr MagnetometerCalibration kMagnetometerCalibration{.hardIronOffsetX = 0.0, .hardIronOffsetY = 0.0};

// TODO(config): static values for this story only - Story 4.1 replaces
// this with the on-flash config file (1-2 networks + token, AD-10), never
// compiled into firmware source in the real deployment.
constexpr const char *kWifiSsid = "CHANGE_ME_SSID";
constexpr const char *kWifiPassword = "CHANGE_ME_PASSWORD";
constexpr const char *kBackendBaseUrl = "http://CHANGE_ME_BACKEND_HOST";
constexpr const char *kIngestToken = "CHANGE_ME_TOKEN";

Anemometer anemometer;
Vane vane;
Magnetometer magnetometer;
WifiTransmitClient transmitClient;
StationClock clock_;
ConnectionMonitor connectionMonitor;
FlashBuffer flashBuffer;
unsigned long lastSampleAt = 0;
unsigned long nextClientSeq = 0;

String makeClientId() {
    return WiFi.macAddress() + "-" + String(nextClientSeq++);
}
} // namespace

void setup() {
    anemometer.begin(kAnemometerPin);
    vane.begin(kVanePin);
    magnetometer.begin();
    pinMode(kDisconnectLedPin, OUTPUT);
    transmitClient.begin(kWifiSsid, kWifiPassword, kBackendBaseUrl, kIngestToken);
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
    double magX, magY;
    magnetometer.readRawXY(magX, magY);
    double yawDegrees = magnetometerHeadingDegrees(magX, magY, kMagnetometerCalibration);
    int windDirOctant = correctWindDirectionOctant(rawVaneOctant, yawDegrees);

    Reading reading;
    reading.clientId = makeClientId().c_str();
    reading.capturedAt = clock_.nowIso8601().c_str();
    reading.clockSynced = clock_.isSynced();
    reading.windSpeedMs = windSpeedMs;
    reading.windDirOctant = windDirOctant;
    reading.rssiValid = false; // wired up in Story 2.5
    reading.rssiDbm = 0;

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
}
