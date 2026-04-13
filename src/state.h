#pragma once
#include <Arduino.h>
#include "config.h"

// Shared application state — defined in main.cpp, extern everywhere else
struct AppState {
    int   rawAdc     = 0;
    float widthMm    = 0.0f;
    bool  alarm      = false;
    float threshLow  = DEFAULT_THRESHOLD_LOW;
    float threshHigh = DEFAULT_THRESHOLD_HIGH;
};

extern AppState g;
