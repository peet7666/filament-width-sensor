#pragma once
#include <Arduino.h>
#include "config.h"

// ADC1 only — ADC2 is unusable when WiFi is active.
// Oversampling reduces noise: ADC_SAMPLES readings averaged.

inline void sensorInit() {
    analogReadResolution(12);           // 0–4095
    analogSetAttenuation(ADC_11db);     // 0–3.3 V input range
}

// Returns averaged raw ADC value (0–4095)
inline int sensorReadRaw() {
    long sum = 0;
    for (int i = 0; i < ADC_SAMPLES; i++) {
        sum += analogRead(PIN_HALL1);
        delayMicroseconds(50);
    }
    return (int)(sum / ADC_SAMPLES);
}
