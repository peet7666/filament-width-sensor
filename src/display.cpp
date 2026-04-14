#include "display.h"
#include <Wire.h>
#include <U8g2lib.h>

// SSD1306 128×64 hardware I2C (custom SDA/SCL defined in config.h)
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
    U8G2_R0,
    /* reset  */ U8X8_PIN_NONE,
    /* clock  */ OLED_SCL,
    /* data   */ OLED_SDA
);

// ─── Layout constants ────────────────────────────────────────────────────────
// 128 × 64 pixels
//  Row 1  Y baseline = 20  — large reading  (font ~18px)
//  Row 2  Y = 26..34       — progress bar
//  Row 3  Y baseline = 46  — threshold labels
//  Row 4  Y baseline = 60  — alarm status

static constexpr int BAR_X  = 4;
static constexpr int BAR_Y  = 26;
static constexpr int BAR_W  = 120;
static constexpr int BAR_H  = 8;

// Map mm value to bar pixel position
static int mmToBarX(float mm) {
    float t = (mm - FILAMENT_MIN) / (FILAMENT_MAX - FILAMENT_MIN);
    t = constrain(t, 0.0f, 1.0f);
    return BAR_X + (int)(t * BAR_W);
}

// ─── Public ──────────────────────────────────────────────────────────────────

void displayInit() {
    Wire.begin(OLED_SDA, OLED_SCL);
    u8g2.begin();
    u8g2.setContrast(200);

    // Splash
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(10, 28, "Filament Width");
    u8g2.drawStr(28, 42, "Sensor v1.0");
    u8g2.sendBuffer();
    delay(1200);
}

void displayUpdate(const AppState& s, bool calibValid) {
    u8g2.clearBuffer();

    if (!calibValid) {
        // ── No calibration yet ───────────────────────────────────────────────
        u8g2.setFont(u8g2_font_9x18B_tf);
        u8g2.drawStr(14, 22, "CALIBRATE!");
        u8g2.setFont(u8g2_font_6x10_tf);
        char buf[24];
        snprintf(buf, sizeof(buf), "r=%.3f", s.ratioAdj);
        u8g2.drawStr(32, 42, buf);
        u8g2.drawStr(8, 58, "Use web interface");
    } else {
        // ── Normal operation ─────────────────────────────────────────────────

        // Row 1 — reading
        char readBuf[12];
        snprintf(readBuf, sizeof(readBuf), "%.2f mm", s.widthMm);
        u8g2.setFont(u8g2_font_9x18B_tf);
        // Centre text (9px per char, max 7 chars "X.XX mm")
        int textW = strlen(readBuf) * 9;
        u8g2.drawStr((128 - textW) / 2, 20, readBuf);

        // Row 2 — progress bar background
        u8g2.drawFrame(BAR_X, BAR_Y, BAR_W, BAR_H);

        // Threshold markers (2px wide lines inside bar)
        int loX = mmToBarX(s.threshLow);
        int hiX = mmToBarX(s.threshHigh);
        u8g2.drawVLine(loX, BAR_Y, BAR_H);
        u8g2.drawVLine(hiX, BAR_Y, BAR_H);

        // Fill bar up to current value
        int fillW = mmToBarX(s.widthMm) - BAR_X;
        if (fillW > 0) {
            u8g2.drawBox(BAR_X + 1, BAR_Y + 1, min(fillW - 1, BAR_W - 2), BAR_H - 2);
        }

        // Row 3 — threshold values
        u8g2.setFont(u8g2_font_6x10_tf);
        char thrBuf[28];
        snprintf(thrBuf, sizeof(thrBuf), "Lo:%.2f    Hi:%.2f", s.threshLow, s.threshHigh);
        u8g2.drawStr(2, 46, thrBuf);

        // Row 4 — alarm status
        if (s.alarm) {
            // Invert background for alarm visibility
            u8g2.setDrawColor(1);
            u8g2.drawBox(0, 50, 128, 14);
            u8g2.setDrawColor(0);
            u8g2.drawStr(28, 61, "!! AVARIA !!");
            u8g2.setDrawColor(1);
        } else {
            u8g2.drawStr(44, 61, "OK");
        }
    }

    u8g2.sendBuffer();
}
