#pragma once
#include <Arduino.h>
#include "config.h"

// Differential Hall-sensor reading.
// Two sensors face each other; filament passes between them.
// ADC1 only channels (ADC2 is unusable when WiFi is active).
// Oversampling reduces noise: ADC_SAMPLES readings averaged per channel.

struct SensorReading {
    int   adc1;    // averaged raw ADC for Hall sensor 1 (PIN_HALL1)
    int   adc2;    // averaged raw ADC for Hall sensor 2 (PIN_HALL2)
    float ratio;   // ratiometric: (adc1-adc2)/(adc1+adc2), range -1..+1
};

inline void sensorInit() {
    analogReadResolution(12);          // 0–4095
    analogSetAttenuation(ADC_11db);    // 0–3.3 V input range
}

inline SensorReading sensorReadBoth() {
    long sum1 = 0, sum2 = 0;
    for (int i = 0; i < ADC_SAMPLES; i++) {
        sum1 += analogRead(PIN_HALL1);
        delayMicroseconds(50);
        sum2 += analogRead(PIN_HALL2);
        delayMicroseconds(50);
    }
    SensorReading r;
    r.adc1 = (int)(sum1 / ADC_SAMPLES);
    r.adc2 = (int)(sum2 / ADC_SAMPLES);
    int denom = r.adc1 + r.adc2;
    r.ratio = (denom != 0) ? (float)(r.adc1 - r.adc2) / (float)denom : 0.0f;
    return r;
}
