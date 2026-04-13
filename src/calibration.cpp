#include "calibration.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

// ─── Mutators ────────────────────────────────────────────────────────────────

bool CalibrationTable::addPoint(int adc, float mm) {
    if (_count >= CALIB_POINTS_MAX) return false;
    // Overwrite if same ADC bucket (±5 counts)
    for (int i = 0; i < _count; i++) {
        if (abs(_points[i].adc - adc) <= 5) {
            _points[i].adc = adc;
            _points[i].mm  = mm;
            sortPoints();
            return true;
        }
    }
    _points[_count++] = {adc, mm};
    sortPoints();
    return true;
}

bool CalibrationTable::removePoint(int index) {
    if (index < 0 || index >= _count) return false;
    for (int i = index; i < _count - 1; i++) {
        _points[i] = _points[i + 1];
    }
    _count--;
    return true;
}

void CalibrationTable::clear() {
    _count = 0;
}

// ─── Interpolation ────────────────────────────────────────────────────────────

float CalibrationTable::interpolate(int adc) const {
    if (_count < 2) return 0.0f;

    // Clamp at edges (no extrapolation — safer for this use case)
    if (adc <= _points[0].adc)         return _points[0].mm;
    if (adc >= _points[_count - 1].adc) return _points[_count - 1].mm;

    // Linear interpolation between adjacent points
    for (int i = 0; i < _count - 1; i++) {
        if (adc >= _points[i].adc && adc <= _points[i + 1].adc) {
            float t = (float)(adc - _points[i].adc)
                    / (float)(_points[i + 1].adc - _points[i].adc);
            return _points[i].mm + t * (_points[i + 1].mm - _points[i].mm);
        }
    }
    return 0.0f;
}

// ─── Persistence ─────────────────────────────────────────────────────────────

bool CalibrationTable::save() const {
    File f = LittleFS.open(CALIB_FILE, "w");
    if (!f) return false;

    JsonDocument doc;
    JsonArray arr = doc["points"].to<JsonArray>();
    for (int i = 0; i < _count; i++) {
        JsonObject p = arr.add<JsonObject>();
        p["adc"] = _points[i].adc;
        p["mm"]  = serialized(String(_points[i].mm, 2));
    }
    serializeJson(doc, f);
    f.close();
    return true;
}

bool CalibrationTable::load() {
    File f = LittleFS.open(CALIB_FILE, "r");
    if (!f) return false;

    JsonDocument doc;
    if (deserializeJson(doc, f) != DeserializationError::Ok) { f.close(); return false; }
    f.close();

    _count = 0;
    for (JsonObject p : doc["points"].as<JsonArray>()) {
        if (_count >= CALIB_POINTS_MAX) break;
        _points[_count++] = { p["adc"].as<int>(), p["mm"].as<float>() };
    }
    sortPoints();
    return true;
}

// ─── Private ─────────────────────────────────────────────────────────────────

void CalibrationTable::sortPoints() {
    // Simple insertion sort (max 20 elements)
    for (int i = 1; i < _count; i++) {
        Point key = _points[i];
        int j = i - 1;
        while (j >= 0 && _points[j].adc > key.adc) {
            _points[j + 1] = _points[j];
            j--;
        }
        _points[j + 1] = key;
    }
}
