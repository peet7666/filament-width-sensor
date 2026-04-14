#include "calibration.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

// ─── Mutators ────────────────────────────────────────────────────────────────

bool CalibrationTable::addPoint(float ratio, float mm) {
    if (_count >= CALIB_POINTS_MAX) return false;
    // Overwrite if within epsilon of an existing point
    for (int i = 0; i < _count; i++) {
        if (fabsf(_points[i].ratio - ratio) <= RATIO_EPSILON) {
            _points[i].ratio = ratio;
            _points[i].mm    = mm;
            sortPoints();
            return true;
        }
    }
    _points[_count++] = {ratio, mm};
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

void CalibrationTable::setRatioZero(float rz) {
    _ratioZero = rz;
    clear();   // existing points are based on old zero → invalidate
}

// ─── Interpolation ────────────────────────────────────────────────────────────

float CalibrationTable::interpolate(float ratio) const {
    if (_count < 2) return 0.0f;

    // Clamp at edges
    if (ratio <= _points[0].ratio)           return _points[0].mm;
    if (ratio >= _points[_count - 1].ratio)  return _points[_count - 1].mm;

    // Linear interpolation between adjacent points
    for (int i = 0; i < _count - 1; i++) {
        if (ratio >= _points[i].ratio && ratio <= _points[i + 1].ratio) {
            float t = (ratio - _points[i].ratio)
                    / (_points[i + 1].ratio - _points[i].ratio);
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
    doc["ratio_zero"] = serialized(String(_ratioZero, 4));
    JsonArray arr = doc["points"].to<JsonArray>();
    for (int i = 0; i < _count; i++) {
        JsonObject p = arr.add<JsonObject>();
        p["ratio"] = serialized(String(_points[i].ratio, 4));
        p["mm"]    = serialized(String(_points[i].mm,    2));
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

    _ratioZero = doc["ratio_zero"] | 0.0f;
    _count = 0;
    for (JsonObject p : doc["points"].as<JsonArray>()) {
        if (_count >= CALIB_POINTS_MAX) break;
        _points[_count++] = { p["ratio"].as<float>(), p["mm"].as<float>() };
    }
    sortPoints();
    return true;
}

// ─── Private ─────────────────────────────────────────────────────────────────

void CalibrationTable::sortPoints() {
    // Insertion sort ascending by ratio
    for (int i = 1; i < _count; i++) {
        Point key = _points[i];
        int j = i - 1;
        while (j >= 0 && _points[j].ratio > key.ratio) {
            _points[j + 1] = _points[j];
            j--;
        }
        _points[j + 1] = key;
    }
}
