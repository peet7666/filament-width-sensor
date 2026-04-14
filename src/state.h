#pragma once
#include <Arduino.h>
#include "config.h"

// Shared application state — defined in main.cpp, extern everywhere else
struct AppState {
    // Raw ADC values from both Hall sensors
    int   rawAdc1    = 0;
    int   rawAdc2    = 0;

    // Ratiometric differential: (adc1-adc2)/(adc1+adc2)
    float ratio      = 0.0f;

    // Zero offset (captured with no filament via zero-calibration)
    float ratioZero  = 0.0f;

    // Adjusted ratio used for interpolation: ratio - ratioZero
    float ratioAdj   = 0.0f;

    // Computed filament width (valid only when calibration table is valid)
    float widthMm    = 0.0f;

    bool  alarm      = false;
    float threshLow  = DEFAULT_THRESHOLD_LOW;
    float threshHigh = DEFAULT_THRESHOLD_HIGH;
};

extern AppState g;
