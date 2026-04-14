#pragma once

// ─── WiFi ────────────────────────────────────────────────────────────────────
// Credentials below are compile-time fallbacks only.
// At runtime the device loads /wifi.json from LittleFS (set via web UI).
// If /wifi.json is absent or STA fails → AP mode: "FilamentSensor" / "12345678"
#define WIFI_SSID_DEFAULT     ""           // leave empty → go straight to AP
#define WIFI_PASSWORD_DEFAULT ""
#define HOSTNAME              "filament-sensor"

// ─── Reset pin ───────────────────────────────────────────────────────────────
// Hold LOW (to GND) at power-on for >= RESET_HOLD_MS to erase WiFi settings.
// GPIO0 = BOOT button on most ESP32 dev boards (already has a pull-up).
#define PIN_RESET         0
#define RESET_HOLD_MS     3000     // 3 s hold required

// ─── WiFi config file (LittleFS) ─────────────────────────────────────────────
#define WIFI_CONFIG_FILE  "/wifi.json"

// ─── ADC ─────────────────────────────────────────────────────────────────────
#define PIN_HALL1     34          // Hall sensor 1 (ADC1_CH6)
#define PIN_HALL2     35          // Hall sensor 2 reserved (ADC1_CH7)
#define ADC_SAMPLES   16          // Oversampling count for noise reduction
#define ADC_MAX       4095        // ESP32 ADC 12-bit

// ─── Outputs ─────────────────────────────────────────────────────────────────
#define PIN_PWM       25          // PWM output: duty 0-255 maps to 1.00-3.00 mm
#define PIN_ALARM     26          // Alarm: HIGH = OK, LOW = out of range
#define PIN_ALARM_INV 27          // Alarm inverted: LOW = OK, HIGH = out of range
#define PWM_CHANNEL   0
#define PWM_FREQ      5000        // Hz
#define PWM_RESOLUTION 8          // bits (0-255)

// ─── Measurement range ───────────────────────────────────────────────────────
#define FILAMENT_MIN  1.00f       // mm
#define FILAMENT_MAX  3.00f       // mm

// ─── Thresholds (defaults, overridden by web UI / stored config) ──────────────
#define DEFAULT_THRESHOLD_LOW   1.65f   // mm — alarm if width < this
#define DEFAULT_THRESHOLD_HIGH  1.85f   // mm — alarm if width > this

// ─── Calibration ─────────────────────────────────────────────────────────────
#define CALIB_POINTS_MIN  3
#define CALIB_POINTS_MAX  20

// ─── OLED ─────────────────────────────────────────────────────────────────────
// Adjust these to match your specific ESP32+OLED module:
//   Heltec WiFi Kit 32:  SDA=4, SCL=15, RST=16
//   Generic I2C module:  SDA=21, SCL=22, RST=-1
#define OLED_SDA   21
#define OLED_SCL   22
#define OLED_RST   -1            // -1 if no reset pin
#define OLED_ADDR  0x3C          // I2C address (0x3C or 0x3D)

// ─── Config file path (LittleFS) ─────────────────────────────────────────────
#define CONFIG_FILE     "/config.json"
#define CALIB_FILE      "/calibration.json"
