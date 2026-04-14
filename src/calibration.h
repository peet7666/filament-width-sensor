#pragma once
#include <Arduino.h>
#include "config.h"

// Calibration table for differential Hall-sensor measurement.
// Each point maps an adjusted ratiometric value → filament diameter in mm.
// ratioAdj = (adc1-adc2)/(adc1+adc2) - ratioZero
//
// Zero calibration (no filament) must be performed first; it sets ratioZero
// and clears existing points (they would be invalid with a new baseline).

class CalibrationTable {
public:
    struct Point {
        float ratio;   // adjusted ratiometric value at calibration time
        float mm;
    };

    // Add a calibration point. Returns false if table is full.
    // Points within RATIO_EPSILON of an existing entry overwrite it.
    bool  addPoint(float ratio, float mm);

    // Remove point by index (0-based). Returns false if out of range.
    bool  removePoint(int index);

    // Clear all calibration points (ratioZero is preserved).
    void  clear();

    // Interpolate (clamp at edges) mm value for given adjusted ratio.
    // Returns 0.0 if fewer than 2 points.
    float interpolate(float ratio) const;

    // Store zero-calibration offset and clear calibration points.
    void  setRatioZero(float rz);
    float getRatioZero() const { return _ratioZero; }

    // Persistence — saves both ratioZero and points to CALIB_FILE.
    bool  save() const;
    bool  load();

    int          count()           const { return _count; }
    bool         isValid()         const { return _count >= CALIB_POINTS_MIN; }
    const Point& getPoint(int i)   const { return _points[i]; }

private:
    static constexpr float RATIO_EPSILON = 0.005f;  // duplicate threshold

    void sortPoints();   // sort ascending by ratio

    Point _points[CALIB_POINTS_MAX];
    int   _count     = 0;
    float _ratioZero = 0.0f;
};
