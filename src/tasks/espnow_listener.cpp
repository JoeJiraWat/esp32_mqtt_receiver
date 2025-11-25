#include "tasks/espnow_listener.h"

#include <Arduino.h>
#include <ctype.h>
#include <string.h>

#if defined(ESP8266)
extern "C" {
#include <espnow.h>
}
#include <ESP8266WiFi.h>
#else
#include <esp_now.h>
#include <WiFi.h>
#endif

#include "config/provisioning_store.h"

namespace tasks::espnow {
namespace {
bool g_initialized = false;
uint32_t g_messagesHandled = 0;

void trimWhitespace(char **start, char **end) {
  while (*start <= *end && isspace(static_cast<unsigned char>(**start))) {
    ++(*start);
  }
  while (*end >= *start && isspace(static_cast<unsigned char>(**end))) {
    **end = '\0';
    --(*end);
  }
}

bool processLine(char *line) {
  if (line == nullptr) {
    return false;
  }

  char *eq = strchr(line, '=');
  if (eq == nullptr) {
    return false;
  }

  char *value = eq + 1;
  char *keyEnd = eq - 1;
  *eq = '\0';

  char *keyStart = line;
  char *valueEnd = value + strlen(value);
  if (valueEnd != value) {
    --valueEnd;
  }

  trimWhitespace(&keyStart, &keyEnd);
  trimWhitespace(&value, &valueEnd);

  if (*keyStart == '\0') {
    return false;
  }

  return provisioning::applyKeyValue(keyStart, value);
}

void parseMessage(const uint8_t *data, int len) {
  if (data == nullptr || len <= 0) {
    return;
  }

  char buffer[256];
  const size_t copyLen = static_cast<size_t>(len) < sizeof(buffer) - 1 ? static_cast<size_t>(len)
                                                                      : sizeof(buffer) - 1;
  memcpy(buffer, data, copyLen);
  buffer[copyLen] = '\0';

  char *savePtr = nullptr;
  char *line = strtok_r(buffer, "\n\r", &savePtr);
  bool handled = false;

  while (line != nullptr) {
    handled |= processLine(line);
    line = strtok_r(nullptr, "\n\r", &savePtr);
  }

  if (handled) {
    ++g_messagesHandled;
    Serial.printf("ESP-NOW provisioning update #%lu applied\n",
                  static_cast<unsigned long>(g_messagesHandled));
  }
}

#if defined(ESP8266)
void onReceive(uint8_t *mac, uint8_t *data, uint8_t len) {
  parseMessage(data, static_cast<int>(len));
}
#else
void onReceive(const uint8_t *mac, const uint8_t *data, int len) {
  parseMessage(data, len);
}
#endif

bool ensureWifiStaMode() {
  if (WiFi.getMode() != WIFI_STA) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
  }
  return true;
}

bool initEspNow() {
  if (!ensureWifiStaMode()) {
    return false;
  }

#if defined(ESP8266)
  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed");
    return false;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(onReceive);
#else
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return false;
  }
  esp_now_register_recv_cb(onReceive);
#endif

  return true;
}

}  // namespace

void init() {
  if (g_initialized) {
    return;
  }

  if (initEspNow()) {
    g_initialized = true;
    Serial.println("ESP-NOW provisioning listener ready");
  }
}

void loop() {
  // No periodic work required; placeholder for future diagnostics.
}

}  // namespace tasks::espnow
