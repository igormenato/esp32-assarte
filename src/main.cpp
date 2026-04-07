#include <Arduino.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

#include "app_config.h"

namespace {

WebServer server(80);
Preferences preferences;

bool doorOpen = AppConfig::DOOR_DEFAULT_OPEN;
bool lightOn = AppConfig::LIGHT_DEFAULT_ON;
bool preferencesReady = false;

constexpr char kPreferencesNamespace[] = "assarte";
constexpr char kDoorStateKey[] = "door_open";
constexpr char kLightStateKey[] = "light_on";

void writeOutput(uint8_t pin, bool activeHigh, bool logicalState) {
  digitalWrite(pin, logicalState == activeHigh ? HIGH : LOW);
}

void applyHardwareState() {
  writeOutput(AppConfig::DOOR_PIN, AppConfig::DOOR_ACTIVE_HIGH, doorOpen);
  writeOutput(AppConfig::LIGHT_PIN, AppConfig::LIGHT_ACTIVE_HIGH, lightOn);
}

String deviceStateJson() {
  String response;
  response.reserve(76);
  response += "{\"door\":\"";
  response += doorOpen ? "open" : "closed";
  response += "\",\"light\":\"";
  response += lightOn ? "on" : "off";
  response += "\",\"doorSource\":\"last_known\"}";
  return response;
}

void sendJson(int statusCode, const String& body) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(statusCode, "application/json", body);
}

void sendState() { sendJson(200, deviceStateJson()); }

const char* methodName(HTTPMethod method) {
  switch (method) {
    case HTTP_GET:
      return "GET";
    case HTTP_POST:
      return "POST";
    default:
      return "OTHER";
  }
}

String currentClientIp() { return server.client().remoteIP().toString(); }

void logRequest(const char* outcome) {
  Serial.printf("%s %s from %s -> %s\n", methodName(server.method()),
                server.uri().c_str(), currentClientIp().c_str(), outcome);
}

void persistState() {
  if (!preferencesReady) {
    return;
  }

  preferences.putBool(kDoorStateKey, doorOpen);
  preferences.putBool(kLightStateKey, lightOn);
}

void loadPersistedState() {
  if (!preferencesReady) {
    return;
  }

  doorOpen = preferences.getBool(kDoorStateKey, AppConfig::DOOR_DEFAULT_OPEN);
  lightOn = preferences.getBool(kLightStateKey, AppConfig::LIGHT_DEFAULT_ON);
  Serial.printf("Loaded persisted state: door=%s, light=%s\n",
                doorOpen ? "open" : "closed", lightOn ? "on" : "off");
}

void setDoorState(bool open) {
  doorOpen = open;
  applyHardwareState();
  persistState();
  Serial.printf("Door state changed: %s\n", doorOpen ? "open" : "closed");
}

void markDoorState(bool open) {
  doorOpen = open;
  persistState();
  Serial.printf("Door last-known state corrected: %s\n",
                doorOpen ? "open" : "closed");
}

void setLightState(bool on) {
  lightOn = on;
  applyHardwareState();
  persistState();
  Serial.printf("Light state changed: %s\n", lightOn ? "on" : "off");
}

void markLightState(bool on) {
  lightOn = on;
  persistState();
  Serial.printf("Light saved state corrected: %s\n", lightOn ? "on" : "off");
}

String contentTypeForPath(const String& path) {
  if (path.endsWith(".html")) return "text/html";
  if (path.endsWith(".css")) return "text/css";
  if (path.endsWith(".js")) return "application/javascript";
  if (path.endsWith(".json")) return "application/json";
  if (path.endsWith(".svg")) return "image/svg+xml";
  if (path.endsWith(".png")) return "image/png";
  if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
  if (path.endsWith(".webp")) return "image/webp";
  if (path.endsWith(".txt")) return "text/plain";
  if (path.endsWith(".ico")) return "image/x-icon";
  return "application/octet-stream";
}

bool isImmutableAsset(const String& path) { return path.startsWith("/_app/immutable/"); }

bool pathLooksLikeAsset(const String& path) {
  const int lastSlash = path.lastIndexOf('/');
  const int lastDot = path.lastIndexOf('.');
  return lastDot > lastSlash;
}

bool streamFile(const String& path) {
  if (!LittleFS.exists(path)) {
    return false;
  }

  File file = LittleFS.open(path, "r");
  if (!file) {
    return false;
  }

  server.sendHeader("Cache-Control",
                    isImmutableAsset(path) ? "public, max-age=31536000, immutable"
                                           : "no-cache");
  server.streamFile(file, contentTypeForPath(path));
  file.close();
  return true;
}

void sendNoContent() { server.send(204); }

void handleNotFound() {
  if (server.uri().startsWith("/api/")) {
    logRequest("404 api");
    sendJson(404, "{\"error\":\"not_found\"}");
    return;
  }

  if (server.method() != HTTP_GET && server.method() != HTTP_HEAD) {
    logRequest("405");
    sendJson(405, "{\"error\":\"method_not_allowed\"}");
    return;
  }

  String path = server.uri();
  if (path == "/") {
    path = "/index.html";
  }

  if (streamFile(path)) {
    logRequest("200 file");
    return;
  }

  if (streamFile(path + "/index.html")) {
    logRequest("200 directory-index");
    return;
  }

  if (!pathLooksLikeAsset(path) && streamFile("/index.html")) {
    logRequest("200 app-fallback");
    return;
  }

  logRequest("404");
  server.send(404, "text/plain", "Not found");
}

void configureRoutes() {
  server.on("/api/state", HTTP_GET, []() {
    logRequest("200 state");
    sendState();
  });

  server.on("/api/door/open", HTTP_POST, []() {
    logRequest("200 door-open");
    setDoorState(true);
    sendState();
  });

  server.on("/api/door/closed", HTTP_POST, []() {
    logRequest("200 door-closed");
    setDoorState(false);
    sendState();
  });

  server.on("/api/door/mark-open", HTTP_POST, []() {
    logRequest("200 door-mark-open");
    markDoorState(true);
    sendState();
  });

  server.on("/api/door/mark-closed", HTTP_POST, []() {
    logRequest("200 door-mark-closed");
    markDoorState(false);
    sendState();
  });

  server.on("/api/light/on", HTTP_POST, []() {
    logRequest("200 light-on");
    setLightState(true);
    sendState();
  });

  server.on("/api/light/off", HTTP_POST, []() {
    logRequest("200 light-off");
    setLightState(false);
    sendState();
  });

  server.on("/api/light/mark-on", HTTP_POST, []() {
    logRequest("200 light-mark-on");
    markLightState(true);
    sendState();
  });

  server.on("/api/light/mark-off", HTTP_POST, []() {
    logRequest("200 light-mark-off");
    markLightState(false);
    sendState();
  });

  server.on("/generate_204", HTTP_GET, sendNoContent);
  server.on("/hotspot-detect.html", HTTP_GET,
            []() { server.send(200, "text/plain", "OK"); });
  server.on("/fwlink", HTTP_GET, sendNoContent);

  server.onNotFound(handleNotFound);
}

void startWifi() {
  const bool hasStationConfig = strlen(AppConfig::STA_SSID) > 0;

  if (hasStationConfig) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(AppConfig::STA_SSID, AppConfig::STA_PASSWORD);
    Serial.printf("Connecting to Wi-Fi SSID %s", AppConfig::STA_SSID);

    const unsigned long startedAt = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - startedAt < AppConfig::WIFI_CONNECT_TIMEOUT_MS) {
      delay(250);
      Serial.print(".");
    }
    Serial.println();
  } else {
    WiFi.mode(WIFI_AP);
  }

  if (WiFi.softAP(AppConfig::AP_SSID, AppConfig::AP_PASSWORD)) {
    Serial.printf("Access point ready: %s (%s)\n", AppConfig::AP_SSID,
                  WiFi.softAPIP().toString().c_str());
  } else {
    Serial.println("Failed to start soft AP");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Station connected: %s\n", WiFi.localIP().toString().c_str());
  } else if (hasStationConfig) {
    Serial.println("Station connection failed, continuing with soft AP only");
  }
}

void startFilesystem() {
  if (!LittleFS.begin(false)) {
    Serial.println("Failed to mount LittleFS");
    return;
  }

  Serial.println("LittleFS mounted");
}

void startPreferences() {
  preferencesReady = preferences.begin(kPreferencesNamespace, false);
  if (!preferencesReady) {
    Serial.println("Failed to open preferences namespace");
    return;
  }

  loadPersistedState();
}

}  // namespace

void setup() {
  Serial.begin(115200);
  startPreferences();

  pinMode(AppConfig::DOOR_PIN, OUTPUT);
  pinMode(AppConfig::LIGHT_PIN, OUTPUT);
  applyHardwareState();

  startFilesystem();
  startWifi();
  configureRoutes();
  server.begin();

  Serial.println("HTTP server started");
}

void loop() { server.handleClient(); }
