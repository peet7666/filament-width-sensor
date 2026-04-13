#include "webserver.h"
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"

static AsyncWebServer server(80);
static AppState*        _state  = nullptr;
static CalibrationTable* _calib = nullptr;

// ─── Helpers ─────────────────────────────────────────────────────────────────

static void sendJson(AsyncWebServerRequest* req, const JsonDocument& doc, int code = 200) {
    String body;
    serializeJson(doc, body);
    req->send(code, "application/json", body);
}

static void sendOk(AsyncWebServerRequest* req) {
    req->send(200, "application/json", "{\"ok\":true}");
}

static void sendError(AsyncWebServerRequest* req, const char* msg, int code = 400) {
    String body = String("{\"ok\":false,\"error\":\"") + msg + "\"}";
    req->send(code, "application/json", body);
}

// Body-capturing POST handler — buffers the full body then calls callback.
// Usage: onPost("/path", handler)
using PostHandler = std::function<void(AsyncWebServerRequest*, const String&)>;

static void onPost(const char* path, PostHandler handler) {
    server.on(path, HTTP_POST,
        [](AsyncWebServerRequest* req) {},   // request complete — body already handled
        nullptr,
        [handler](AsyncWebServerRequest* req, uint8_t* data, size_t len,
                  size_t index, size_t total) {
            // For our small JSON payloads, all data arrives in one chunk.
            String body((char*)data, len);
            handler(req, body);
        }
    );
}

// ─── Route handlers ──────────────────────────────────────────────────────────

// GET /api/status → { mm, adc, alarm, pwm_duty }
static void handleGetStatus(AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["mm"]    = serialized(String(_state->widthMm, 2));
    doc["adc"]   = _state->rawAdc;
    doc["alarm"] = _state->alarm;
    sendJson(req, doc);
}

// GET /api/config → { threshold_low, threshold_high }
static void handleGetConfig(AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["threshold_low"]  = serialized(String(_state->threshLow,  2));
    doc["threshold_high"] = serialized(String(_state->threshHigh, 2));
    sendJson(req, doc);
}

// POST /api/config ← { threshold_low, threshold_high }
static void handlePostConfig(AsyncWebServerRequest* req, const String& body) {
    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) {
        sendError(req, "invalid JSON"); return;
    }
    float lo = doc["threshold_low"]  | _state->threshLow;
    float hi = doc["threshold_high"] | _state->threshHigh;
    if (lo >= hi || lo < FILAMENT_MIN || hi > FILAMENT_MAX) {
        sendError(req, "invalid thresholds"); return;
    }
    _state->threshLow  = lo;
    _state->threshHigh = hi;

    // Persist thresholds into a small JSON file alongside calibration
    File f = LittleFS.open(CONFIG_FILE, "w");
    if (f) {
        JsonDocument cfg;
        cfg["threshold_low"]  = serialized(String(lo, 2));
        cfg["threshold_high"] = serialized(String(hi, 2));
        serializeJson(cfg, f);
        f.close();
    }
    sendOk(req);
}

// GET /api/calibration → { points: [{adc, mm}, …] }
static void handleGetCalib(AsyncWebServerRequest* req) {
    JsonDocument doc;
    JsonArray arr = doc["points"].to<JsonArray>();
    for (int i = 0; i < _calib->count(); i++) {
        JsonObject p = arr.add<JsonObject>();
        p["adc"] = _calib->getPoint(i).adc;
        p["mm"]  = serialized(String(_calib->getPoint(i).mm, 2));
    }
    sendJson(req, doc);
}

// POST /api/calibration/add ← { mm: 1.75 }  — uses current ADC from state
static void handleAddCalib(AsyncWebServerRequest* req, const String& body) {
    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) {
        sendError(req, "invalid JSON"); return;
    }
    float mm = doc["mm"] | -1.0f;
    if (mm < FILAMENT_MIN || mm > FILAMENT_MAX) {
        sendError(req, "mm out of range"); return;
    }
    if (!_calib->addPoint(_state->rawAdc, mm)) {
        sendError(req, "table full"); return;
    }
    _calib->save();
    sendOk(req);
}

// POST /api/calibration/del ← { index: 2 }
static void handleDelCalib(AsyncWebServerRequest* req, const String& body) {
    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) {
        sendError(req, "invalid JSON"); return;
    }
    int idx = doc["index"] | -1;
    if (!_calib->removePoint(idx)) {
        sendError(req, "index out of range"); return;
    }
    _calib->save();
    sendOk(req);
}

// POST /api/calibration/clear
static void handleClearCalib(AsyncWebServerRequest* req, const String& body) {
    _calib->clear();
    _calib->save();
    sendOk(req);
}

// ─── Public ──────────────────────────────────────────────────────────────────

void webServerBegin(AppState* state, CalibrationTable* calib) {
    _state = state;
    _calib = calib;

    // Serve static files from LittleFS (index.html, etc.)
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    // API routes
    server.on("/api/status",      HTTP_GET, handleGetStatus);
    server.on("/api/config",      HTTP_GET, handleGetConfig);
    server.on("/api/calibration", HTTP_GET, handleGetCalib);

    onPost("/api/config",             handlePostConfig);
    onPost("/api/calibration/add",    handleAddCalib);
    onPost("/api/calibration/del",    handleDelCalib);
    onPost("/api/calibration/clear",  handleClearCalib);

    server.onNotFound([](AsyncWebServerRequest* req) {
        req->send(404, "text/plain", "Not found");
    });

    server.begin();
    Serial.println("[web] server started on port 80");
}
