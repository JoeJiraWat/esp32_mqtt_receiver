#include "tasks/wifi_task.h"

#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#include "config/provisioning_store.h"

namespace tasks::wifi {
namespace {
constexpr uint32_t WIFI_RETRY_DELAY_MS = 5000;

provisioning::WifiCredentials g_activeCreds{};
uint32_t g_lastAttemptMs = 0;
uint32_t g_lastWifiVersion = 0;
bool g_hasCredentials = false;
bool g_connectionInFlight = false;
bool g_connected = false;

void configureStation() {
#if defined(ESP8266)
  WiFi.persistent(false);
#endif
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.disconnect();
}

void refreshCredentials() {
  g_activeCreds = provisioning::wifi();
  g_hasCredentials = g_activeCreds.valid && g_activeCreds.ssid[0] != '\0';
}

void logCredentialsMissing() {
  if (!g_hasCredentials) {
    Serial.println("WiFi credentials unavailable; waiting for provisioning");
  }
}

void beginConnectionAttempt() {
  if (!g_hasCredentials) {
    return;
  }

  Serial.printf("Connecting to WiFi SSID '%s'\n", g_activeCreds.ssid);
  WiFi.begin(g_activeCreds.ssid, g_activeCreds.password);
  g_lastAttemptMs = millis();
  g_connectionInFlight = true;
}

void handleCredentialUpdates() {
  const uint32_t version = provisioning::wifiVersion();
  if (version == g_lastWifiVersion) {
    return;
  }

  g_lastWifiVersion = version;
  refreshCredentials();
  g_connected = false;
  g_connectionInFlight = false;
  WiFi.disconnect();
  if (g_hasCredentials) {
    beginConnectionAttempt();
  } else {
    logCredentialsMissing();
  }
}

}  // namespace

void init() {
  configureStation();
  refreshCredentials();
  g_lastWifiVersion = provisioning::wifiVersion();

  if (!g_hasCredentials) {
    logCredentialsMissing();
    return;
  }

  beginConnectionAttempt();
}

void loop() {
  handleCredentialUpdates();

  if (!g_hasCredentials) {
    return;
  }

  const wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    if (!g_connected) {
      g_connected = true;
      g_connectionInFlight = false;
      Serial.printf("WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());
    }
    return;
  }

  if (g_connected) {
    g_connected = false;
    Serial.println("WiFi disconnected");
  }

  const uint32_t now = millis();
  if (!g_connectionInFlight || (now - g_lastAttemptMs) >= WIFI_RETRY_DELAY_MS) {
    beginConnectionAttempt();
  }
}

bool isConnected() { return WiFi.status() == WL_CONNECTED; }

void requestReconnect() {
  g_connectionInFlight = false;
  g_connected = false;
  WiFi.disconnect();
}

}  // namespace tasks::wifi
