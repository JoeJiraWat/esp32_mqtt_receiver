#pragma once

#include <stdint.h>

namespace provisioning {
struct WifiCredentials {
  char ssid[33];
  char password[65];
  bool valid;
};

struct MqttInitParams {
  char host[65];
  uint16_t port;
  char clientId[33];
  char username[33];
  char password[65];
  char publishTopic[65];
  char commandTopic[65];
  char heartbeatTopic[65];
  bool valid;
};

void initDefaults();

WifiCredentials wifi();
MqttInitParams mqtt();

bool hasWifiCredentials();
bool hasMqttParams();
bool applyKeyValue(const char *key, const char *value);
uint32_t wifiVersion();
uint32_t mqttVersion();
}  // namespace provisioning
