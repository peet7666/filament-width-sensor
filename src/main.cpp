#include <Arduino.h>
#include "config.h"

// ─── Module stubs (to be implemented) ────────────────────────────────────────
// #include "sensor.h"       // ADC reading + oversampling
// #include "calibration.h"  // Calibration table + interpolation
// #include "display.h"      // OLED rendering
// #include "webserver.h"    // Async HTTP server (config + calibration UI)

void setup() {
    Serial.begin(115200);

    // TODO: LittleFS init + load config/calibration
    // TODO: WiFi connect
    // TODO: OLED init
    // TODO: Web server init
    // TODO: ADC + output pin init
}

void loop() {
    // TODO: read ADC → interpolate calibration → update display + outputs
}
