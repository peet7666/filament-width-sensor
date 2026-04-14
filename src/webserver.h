#pragma once
#include "state.h"
#include "calibration.h"

// Start the async HTTP server.
// Pass pointers to the shared state and calibration table so handlers can
// read/write them directly.
void webServerBegin(AppState* state, CalibrationTable* calib);

// Returns true if a WiFi-settings save was received and the device should reboot.
bool webServerPendingRestart();
