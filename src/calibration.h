#pragma once
#include <Arduino.h>
#include "config.h"

class CalibrationTable {
public:
    struct Point {
        int   adc;
        float mm;
    };

    // Add a calibration point. Returns false if table is full or duplicate ADC.
    bool  addPoint(int adc, float mm);
    // Remove point by index (0-based). Returns false if out of range.
    bool  removePoint(int index);
    // Clear all points.
    void  clear();

    // Interpolate (or extrapolate/clamp at edges) mm value for given raw ADC.
    // Returns 0.0 if fewer than 2 points.
    float interpolate(int adc) const;

    // Persistence
    bool  save() const;   // write to LittleFS (CALIB_FILE)
    bool  load();         // read from LittleFS; returns false if file missing

    int         count()            const { return _count; }
    bool        isValid()          const { return _count >= CALIB_POINTS_MIN; }
    const Point& getPoint(int i)   const { return _points[i]; }

private:
    void  sortPoints();   // sort ascending by ADC value

    Point _points[CALIB_POINTS_MAX];
    int   _count = 0;
};
