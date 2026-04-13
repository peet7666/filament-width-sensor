#pragma once
#include <Arduino.h>
#include "config.h"

inline void outputsInit() {
    // PWM — LEDC channel
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PIN_PWM, PWM_CHANNEL);

    // Alarm outputs — default to OK state
    pinMode(PIN_ALARM,     OUTPUT);
    pinMode(PIN_ALARM_INV, OUTPUT);
    digitalWrite(PIN_ALARM,     HIGH);   // HIGH = OK
    digitalWrite(PIN_ALARM_INV, LOW);    // LOW  = OK
}

// Call every loop after interpolation.
// mm = 0 means calibration invalid — treat as alarm.
inline void outputsUpdate(float mm, float threshLow, float threshHigh) {
    bool alarm = (mm <= 0.0f) || (mm < threshLow) || (mm > threshHigh);

    // PWM duty: 0% → 1.00 mm, 100% → 3.00 mm (clamped)
    float clamped = constrain(mm, FILAMENT_MIN, FILAMENT_MAX);
    int duty = (int)((clamped - FILAMENT_MIN) / (FILAMENT_MAX - FILAMENT_MIN) * 255.0f);
    ledcWrite(PWM_CHANNEL, duty);

    // Alarm pins
    digitalWrite(PIN_ALARM,     alarm ? LOW  : HIGH);
    digitalWrite(PIN_ALARM_INV, alarm ? HIGH : LOW);
}
