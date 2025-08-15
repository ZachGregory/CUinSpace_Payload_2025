#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <esp_wifi.h>
#include <esp_task_wdt.h>

// ====== AP CONFIG ======
static const char* AP_SSID     = "inSpace-Payload";
static const char* AP_PASSWORD = "launchcanada"; // 8+ chars
static const int   AP_CHANNEL  = 6;
static const bool  AP_HIDDEN   = false;
static const int   AP_MAX_CONN = 6;

// Track AP state
volatile bool g_apUp = false;

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

// Forward-declare
void startAP();

// Wi-Fi event log + recovery
void onWiFiEvent(WiFiEvent_t event) {
  switch(event) {
#if defined(ARDUINO_EVENT_WIFI_AP_START)
    case ARDUINO_EVENT_WIFI_AP_START:
#elif defined(SYSTEM_EVENT_AP_START)
    case SYSTEM_EVENT_AP_START:
#endif
      g_apUp = true;
      Serial.println("[WiFi] AP START");
      break;

#if defined(ARDUINO_EVENT_WIFI_AP_STOP)
    case ARDUINO_EVENT_WIFI_AP_STOP:
#elif defined(SYSTEM_EVENT_AP_STOP)
    case SYSTEM_EVENT_AP_STOP:
#endif
      g_apUp = false;
      Serial.println("[WiFi] AP STOP — restarting…");
      startAP();
      break;

#if defined(ARDUINO_EVENT_WIFI_AP_STACONNECTED)
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
#elif defined(SYSTEM_EVENT_AP_STACONNECTED)
    case SYSTEM_EVENT_AP_STACONNECTED:
#endif
      Serial.printf("[WiFi] STA++ now=%d\n", WiFi.softAPgetStationNum());
      break;

#if defined(ARDUINO_EVENT_WIFI_AP_STADISCONNECTED)
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
#elif defined(SYSTEM_EVENT_AP_STADISCONNECTED)
    case SYSTEM_EVENT_AP_STADISCONNECTED:
#endif
      Serial.printf("[WiFi] STA-- now=%d\n", WiFi.softAPgetStationNum());
      break;

    default: break;
  }
}

void startAP() {
  WiFi.mode(WIFI_AP);
  // Optional: fix IP to the classic 192.168.4.1
  WiFi.softAPConfig(IPAddress(192,168,4,1),
                    IPAddress(192,168,4,1),
                    IPAddress(255,255,255,0));

  bool ok = WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, AP_HIDDEN, AP_MAX_CONN);
  g_apUp = ok;
  Serial.printf("[WiFi] AP %s  SSID:%s  IP:%s\n",
                ok ? "UP" : "FAILED",
                AP_SSID, WiFi.softAPIP().toString().c_str());
}

// ====== SERVER ======
AsyncWebServer server(80);
AsyncEventSource events("/events");

// Optional snapshot endpoint (handy for debugging)
static void handleData(AsyncWebServerRequest* req) {
  auto r = makeFake();
  req->send(200, "application/json", toJson(r));
}

// Background task pushing SSE at 1 Hz
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

// Simple watchdog: every 5 s ensure AP is up.
void apWatchdogTask(void*){
  const TickType_t period = pdMS_TO_TICKS(5000);
  TickType_t last = xTaskGetTickCount();
  for(;;){
    vTaskDelayUntil(&last, period);
    if (!g_apUp) {
      Serial.println("[WiFi] Watchdog: AP down -> restarting");
      startAP();
    }
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

  // *** Wi-Fi radio hardening ***
  WiFi.persistent(false);                // don’t write NVS repeatedly
  WiFi.setSleep(false);                  // Arduino API (keeps radio awake)
  esp_wifi_set_ps(WIFI_PS_NONE);         // IDF API (disable power-save)
  esp_wifi_set_country_code("CA", true); // ensure 1–11 channels, Canada

  // Event handler for AP up/down
  WiFi.onEvent(onWiFiEvent);

  // Start AP
  startAP();

  // Static site from LittleFS root; index.html = default
  server.serveStatic("/", LittleFS, "/")
        .setDefaultFile("index.html")
        .setCacheControl("public,max-age=31536000,immutable"); // great when you gzip/version files

  // SSE endpoint
  server.addHandler(&events);

  // Healthcheck
  server.on("/api/ping", HTTP_GET, [](AsyncWebServerRequest* r){ r->send(200,"text/plain","ok"); });

  // 404
  server.onNotFound([](AsyncWebServerRequest* r){
    r->send(404, "text/plain", "Not found. Try /");
  });

  server.begin();

  // Start telemetry task on core 1
  xTaskCreatePinnedToCore(telemetryTask, "telemetry", 4096, nullptr, 1, nullptr, 1);

  // Start watchdog timer
  xTaskCreatePinnedToCore(apWatchdogTask, "apwatch", 3072, nullptr, 1, nullptr, 1);

  Serial.println("HTTP server ready. Open http://192.168.4.1");
}

void loop() {
  // No polling needed — it's all async.
}
