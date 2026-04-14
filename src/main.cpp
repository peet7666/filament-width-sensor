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
static constexpr uint32_t WIFI_TIMEOUT_MS = 12000;

// Loads WiFi credentials + optional static IP from /wifi.json.
// Returns true if ssid is non-empty.
static bool loadWifiConfig(String& ssid, String& password,
                           String& staticIp, String& gateway, String& subnet) {
    File f = LittleFS.open(WIFI_CONFIG_FILE, "r");
    if (!f) return false;
    JsonDocument doc;
    bool ok = (deserializeJson(doc, f) == DeserializationError::Ok);
    f.close();
    if (!ok) return false;
    ssid     = doc["ssid"]      | "";
    password = doc["password"]  | "";
    staticIp = doc["static_ip"] | "";
    gateway  = doc["gateway"]   | "";
    subnet   = doc["subnet"]    | "";
    return ssid.length() > 0;
}

static void wifiConnect() {
    String ssid, password, staticIp, gateway, subnet;

    if (!loadWifiConfig(ssid, password, staticIp, gateway, subnet)) {
        // No saved config → AP mode immediately
        WiFi.mode(WIFI_AP);
        WiFi.softAP("FilamentSensor", "12345678");
        Serial.println("[wifi] no config — AP mode. Connect to 'FilamentSensor', open 192.168.4.1");
        return;
    }

    // Optional static IP
    if (staticIp.length() > 0) {
        IPAddress ip, gw, sn;
        if (ip.fromString(staticIp) && gw.fromString(gateway) && sn.fromString(subnet)) {
            WiFi.config(ip, gw, sn);
            Serial.printf("[wifi] static IP: %s\n", staticIp.c_str());
        }
    }

    Serial.printf("[wifi] connecting to %s", ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOSTNAME);
    WiFi.begin(ssid.c_str(), password.c_str());

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) {
        delay(300);
        Serial.print('.');
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[wifi] connected — IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\n[wifi] STA failed — AP fallback");
        WiFi.mode(WIFI_AP);
        WiFi.softAP("FilamentSensor", "12345678");
        Serial.println("[wifi] AP mode — connect to 'FilamentSensor', open 192.168.4.1");
    }
}

// ─── Reset pin ────────────────────────────────────────────────────────────────
// Hold PIN_RESET LOW for RESET_HOLD_MS at boot → erase WiFi config.
static void checkResetPin() {
    pinMode(PIN_RESET, INPUT_PULLUP);
    if (digitalRead(PIN_RESET) == HIGH) return;   // not pressed

    Serial.print("[reset] hold detected, waiting...");
    uint32_t start = millis();
    while (digitalRead(PIN_RESET) == LOW) {
        if (millis() - start >= RESET_HOLD_MS) {
            LittleFS.remove(WIFI_CONFIG_FILE);
            Serial.println("\n[reset] WiFi config erased! Rebooting...");
            delay(500);
            ESP.restart();
        }
        delay(100);
        Serial.print('.');
    }
    Serial.println("\n[reset] released too early, ignored");
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

    // Reset pin check (before WiFi init)
    checkResetPin();

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
static constexpr uint32_t SERIAL_INTERVAL_MS   = 1000;  // serial log rate

static uint32_t lastMeasure = 0;
static uint32_t lastDisplay = 0;
static uint32_t lastSerial  = 0;

void loop() {
    // Reboot requested by web WiFi-settings handler
    if (webServerPendingRestart()) {
        Serial.println("[wifi] new settings saved — rebooting...");
        delay(500);
        ESP.restart();
    }

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

    // ── Serial log ────────────────────────────────────────────────────────────
    if (now - lastSerial >= SERIAL_INTERVAL_MS) {
        lastSerial = now;
        if (calib.isValid()) {
            Serial.printf("[sensor] %.2f mm  ADC=%d  %s\n",
                          g.widthMm, g.rawAdc, g.alarm ? "ALARM" : "OK");
        } else {
            Serial.printf("[sensor] ADC=%d  (no calibration)\n", g.rawAdc);
        }
    }
}
