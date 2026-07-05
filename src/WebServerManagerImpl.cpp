#include <Arduino.h>
#include <WiFi.h>
#include <SD.h>
#include "WebServerManagerImpl.h"
#include "../include/Config.h"

static const char* WIFI_AP_SSID = "hippySafari";
static const char* WIFI_AP_PASSWORD = "1234safari";

bool WebServerManagerImpl::initialize() {
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD)) {
    snprintf(lastError, sizeof(lastError), "softAP() failed");
    return false;
  }

  Serial.print("[WEB] AP started, SSID=");
  Serial.print(WIFI_AP_SSID);
  Serial.print(" IP=");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, [this]() { handleRoot(); });
  server.on("/logs/list", HTTP_GET, [this]() { handleLogList(); });
  server.onNotFound([this]() { handleFile(); });

  server.begin();
  Serial.println("[WEB] HTTP server started on port 80");
  return true;
}

void WebServerManagerImpl::update() {
  server.handleClient();
}

const char* WebServerManagerImpl::getLastError() const {
  return lastError;
}

static const char* mimeTypeFor(const String& name) {
  if (name.endsWith(".html")) return "text/html";
  if (name.endsWith(".js")) return "application/javascript";
  if (name.endsWith(".json")) return "application/json";
  return "text/plain";
}

void WebServerManagerImpl::serveLogsFile(const char* name) {
  String path = String(PATH_LOGS_ROOT) + "/" + name;
  if (!SD.exists(path)) {
    server.send(404, "text/plain", "Not found");
    return;
  }
  File f = SD.open(path);
  if (!f) {
    server.send(500, "text/plain", "Failed to open file");
    return;
  }
  server.streamFile(f, mimeTypeFor(path));
  f.close();
}

void WebServerManagerImpl::handleRoot() {
  serveLogsFile("index.html");
}

void WebServerManagerImpl::handleFile() {
  // Requests come in as "/index.html", "/highcharts.js", "/2026-07-05.json", etc.
  // (index.html references these as relative paths, e.g. "./highcharts.js").
  String uri = server.uri();
  if (uri.startsWith("/")) uri.remove(0, 1);
  if (uri.length() == 0) {
    handleRoot();
    return;
  }
  serveLogsFile(uri.c_str());
}

void WebServerManagerImpl::handleLogList() {
  File dir = SD.open(PATH_LOGS_ROOT);
  if (!dir) {
    server.send(500, "application/json", "[]");
    return;
  }

  String json = "[";
  bool first = true;
  File entry = dir.openNextFile();
  while (entry) {
    String name = entry.name();
    int slash = name.lastIndexOf('/');
    if (slash >= 0) name = name.substring(slash + 1);

    if (!entry.isDirectory() && name.endsWith(".json") && name != "keymap.json") {
      if (!first) json += ",";
      json += "\"" + name + "\"";
      first = false;
    }
    entry.close();
    entry = dir.openNextFile();
  }
  dir.close();
  json += "]";

  server.send(200, "application/json", json);
}
