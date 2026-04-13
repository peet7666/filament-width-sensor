#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include "config.h"
#include "state.h"
#include "sensor.h"
#include "calibration.h"
#include "outputs.h"
#include "display.h"
#include "webserver.h"

// ─── Global state ─────────────────────────────────────────────────────────────
AppState         g;
CalibrationTable calib;

// ─── WiFi ─────────────────────────────────────────────────────────────────────
// Tries to connect to the stored SSID.
// Falls back to AP mode ("FilamentSensor" / "12345678") after WIFI_TIMEOUT_MS.
static constexpr uint32_t WIFI_TIMEOUT_MS = 12000;

static void wifiConnect() {
    Serial.printf("[wifi] connecting to %s", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) {
        delay(300);
        Serial.print('.');
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[wifi] connected — IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        // AP fallback
        WiFi.mode(WIFI_AP);
        WiFi.softAP("FilamentSensor", "12345678");
        Serial.printf("\n[wifi] AP mode — connect to 'FilamentSensor', open 192.168.4.1\n");
    }
}

// ─── Config load ──────────────────────────────────────────────────────────────
static void loadConfig() {
    File f = LittleFS.open(CONFIG_FILE, "r");
    if (!f) return;
    JsonDocument doc;
    if (deserializeJson(doc, f) == DeserializationError::Ok) {
        g.threshLow  = doc["threshold_low"]  | DEFAULT_THRESHOLD_LOW;
        g.threshHigh = doc["threshold_high"] | DEFAULT_THRESHOLD_HIGH;
    }
    f.close();
}

// ─── Setup ────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial.println("\n[boot] Filament Width Sensor");

    // Filesystem
    if (!LittleFS.begin(true)) {         // true = format if mount fails
        Serial.println("[fs] LittleFS mount failed!");
    }

    // Load saved thresholds and calibration
    loadConfig();
    calib.load();
    Serial.printf("[calib] loaded %d points (valid: %s)\n",
                  calib.count(), calib.isValid() ? "yes" : "no");

    // Hardware init
    sensorInit();
    outputsInit();
    displayInit();

    // WiFi + web server
    wifiConnect();
    webServerBegin(&g, &calib);
}

// ─── Loop ─────────────────────────────────────────────────────────────────────
static constexpr uint32_t MEASURE_INTERVAL_MS  = 100;   // sensor poll rate
static constexpr uint32_t DISPLAY_INTERVAL_MS  = 200;   // OLED refresh rate

static uint32_t lastMeasure = 0;
static uint32_t lastDisplay = 0;

void loop() {
    uint32_t now = millis();

    // ── Measurement ───────────────────────────────────────────────────────────
    if (now - lastMeasure >= MEASURE_INTERVAL_MS) {
        lastMeasure = now;

        g.rawAdc  = sensorReadRaw();
        g.widthMm = calib.isValid() ? calib.interpolate(g.rawAdc) : 0.0f;
        g.alarm   = !calib.isValid()
                 || g.widthMm < g.threshLow
                 || g.widthMm > g.threshHigh;

        outputsUpdate(g.widthMm, g.threshLow, g.threshHigh);
    }

    // ── Display ───────────────────────────────────────────────────────────────
    if (now - lastDisplay >= DISPLAY_INTERVAL_MS) {
        lastDisplay = now;
        displayUpdate(g, calib.isValid());
    }
}
