#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

// ====== AP CONFIG ======
static const char* AP_SSID     = "inSpace-Payload";
static const char* AP_PASSWORD = "launchcanada"; // 8+ chars
static const int   AP_CHANNEL  = 6;
static const bool  AP_HIDDEN   = false;
static const int   AP_MAX_CONN = 6;

// ====== SERVER ======
AsyncWebServer server(80);
AsyncEventSource events("/events");

// ====== TELEMETRY MODEL ======
struct Reading {
  float t, h, p, ax, ay, az;
};

static Reading makeFake() {
  auto rf = [](float a, float b){
    return a + (b - a) * (random(0,10000) / 10000.0f);
  };
  Reading r;
  r.t  = rf(18.0f, 35.0f);
  r.h  = rf(25.0f, 65.0f);
  r.p  = rf(980.0f, 1030.0f);
  r.ax = rf(-0.05f, 0.05f);
  r.ay = rf(-0.05f, 0.05f);
  r.az = rf(0.95f, 1.05f);
  return r;
}

static String toJson(const Reading& r) {
  String j = "{";
  j += "\"uptime_ms\":"     + String(millis());
  j += ",\"temperature_c\":" + String(r.t, 2);
  j += ",\"humidity_pct\":"  + String(r.h, 2);
  j += ",\"pressure_hpa\":"  + String(r.p, 2);
  j += ",\"accel_g\":["      + String(r.ax,3) + "," + String(r.ay,3) + "," + String(r.az,3) + "]";
  j += "}";
  return j;
}

// Optional snapshot endpoint (handy for debugging)
static void handleData(AsyncWebServerRequest* req) {
  auto r = makeFake();
  req->send(200, "application/json", toJson(r));
}

// Background task pushing SSE at 5 Hz
void telemetryTask(void*){
  const TickType_t period = pdMS_TO_TICKS(1000);
  TickType_t last = xTaskGetTickCount();
  for(;;){
    vTaskDelayUntil(&last, period);
    Reading r = makeFake();
    String payload = toJson(r);
    events.send(payload.c_str(), "telemetry", (uint32_t)millis());
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  randomSeed(esp_random());

  // Filesystem
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed (formatted).");
  }

  // AP mode
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, AP_HIDDEN, AP_MAX_CONN);
  Serial.printf("AP start: %s\n", ok ? "OK" : "FAILED");
  Serial.printf("SSID: %s  PASS: %s\n", AP_SSID, AP_PASSWORD);
  Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());

  // Static site from LittleFS root; index.html = default
  server.serveStatic("/", LittleFS, "/")
        .setDefaultFile("index.html")
        .setCacheControl("public,max-age=31536000,immutable"); // great when you gzip/version files

  // SSE endpoint
  server.addHandler(&events);

  // Optional snapshot
  server.on("/api/data", HTTP_GET, handleData);

  // Healthcheck
  server.on("/api/ping", HTTP_GET, [](AsyncWebServerRequest* r){ r->send(200,"text/plain","ok"); });

  // 404
  server.onNotFound([](AsyncWebServerRequest* r){
    r->send(404, "text/plain", "Not found. Try /");
  });

  server.begin();

  // Start telemetry task on core 1
  xTaskCreatePinnedToCore(telemetryTask, "telemetry", 4096, nullptr, 1, nullptr, 1);

  Serial.println("HTTP server ready. Open http://192.168.4.1");
}

void loop() {
  // No polling needed — it's all async.
}
