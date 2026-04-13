#pragma once
#include <Arduino.h>
#include "config.h"
#include "state.h"

// Initialise the OLED and draw the first frame.
void displayInit();

// Call every loop. Renders the current AppState.
// calib_valid — pass calibration.isValid() to show "CALIBRATE" message when needed.
void displayUpdate(const AppState& state, bool calibValid);
