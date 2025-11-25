#include "config/provisioning_store.h"

#include <cstdlib>
#include <cstring>
#include <strings.h>

#include "config/defaults.h"

namespace {
using provisioning::MqttInitParams;
using provisioning::WifiCredentials;

WifiCredentials g_wifi{};
MqttInitParams g_mqtt{};
uint32_t g_wifiVersion = 0;
uint32_t g_mqttVersion = 0;

template <size_t N>
void copyBounded(char (&dest)[N], const char *src) {
  if (src == nullptr) {
    dest[0] = '\0';
    return;
  }

  strncpy(dest, src, N - 1);
  dest[N - 1] = '\0';
}

bool strEqualsIgnoreCase(const char *lhs, const char *rhs) {
  if (lhs == nullptr || rhs == nullptr) {
    return false;
  }
  return strcasecmp(lhs, rhs) == 0;
}

void markWifiUpdated() {
  ++g_wifiVersion;
  g_wifi.valid = g_wifi.ssid[0] != '\0';
}

void markMqttUpdated() {
  ++g_mqttVersion;
  g_mqtt.valid = g_mqtt.host[0] != '\0';
}

}  // namespace

namespace provisioning {
void initDefaults() {
  copyBounded(g_wifi.ssid, WIFI_DEFAULT_SSID);
  copyBounded(g_wifi.password, WIFI_DEFAULT_PASSWORD);
  markWifiUpdated();

  copyBounded(g_mqtt.host, MQTT_HOST);
  g_mqtt.port = MQTT_PORT;
  copyBounded(g_mqtt.clientId, MQTT_CLIENT_ID);
  copyBounded(g_mqtt.username, MQTT_USERNAME);
  copyBounded(g_mqtt.password, MQTT_PASSWORD);
  copyBounded(g_mqtt.publishTopic, MQTT_PUB_TOPIC);
  copyBounded(g_mqtt.commandTopic, MQTT_CMD_TOPIC);
  copyBounded(g_mqtt.heartbeatTopic, MQTT_HEARTBEAT_TOPIC);
  markMqttUpdated();
}

WifiCredentials wifi() {
  WifiCredentials snapshot;
  snapshot = g_wifi;
  return snapshot;
}

MqttInitParams mqtt() {
  MqttInitParams snapshot;
  snapshot = g_mqtt;
  return snapshot;
}

bool hasWifiCredentials() {
  bool valid = false;
  valid = g_wifi.valid;
  return valid;
}

bool hasMqttParams() {
  bool valid = false;
  valid = g_mqtt.valid;
  return valid;
}

uint32_t wifiVersion() {
  uint32_t version = 0;
  version = g_wifiVersion;
  return version;
}

uint32_t mqttVersion() {
  uint32_t version = 0;
  version = g_mqttVersion;
  return version;
}

bool applyKeyValue(const char *key, const char *value) {
  if (key == nullptr || value == nullptr) {
    return false;
  }

  bool handled = false;
  bool wifiTouched = false;
  bool mqttTouched = false;

  if (strEqualsIgnoreCase(key, "WIFI_SSID")) {
    copyBounded(g_wifi.ssid, value);
    wifiTouched = true;
    handled = true;
  } else if (strEqualsIgnoreCase(key, "WIFI_PASSWORD")) {
    copyBounded(g_wifi.password, value);
    wifiTouched = true;
    handled = true;
  } else if (strEqualsIgnoreCase(key, "MQTT_HOST")) {
    copyBounded(g_mqtt.host, value);
    mqttTouched = true;
    handled = true;
  } else if (strEqualsIgnoreCase(key, "MQTT_PORT")) {
    g_mqtt.port = static_cast<uint16_t>(atoi(value));
    mqttTouched = true;
    handled = true;
  } else if (strEqualsIgnoreCase(key, "MQTT_CLIENT_ID")) {
    copyBounded(g_mqtt.clientId, value);
    mqttTouched = true;
    handled = true;
  } else if (strEqualsIgnoreCase(key, "MQTT_USERNAME")) {
    copyBounded(g_mqtt.username, value);
    mqttTouched = true;
    handled = true;
  } else if (strEqualsIgnoreCase(key, "MQTT_PASSWORD")) {
    copyBounded(g_mqtt.password, value);
    mqttTouched = true;
    handled = true;
  } else if (strEqualsIgnoreCase(key, "MQTT_PUB_TOPIC")) {
    copyBounded(g_mqtt.publishTopic, value);
    mqttTouched = true;
    handled = true;
  } else if (strEqualsIgnoreCase(key, "MQTT_CMD_TOPIC")) {
    copyBounded(g_mqtt.commandTopic, value);
    mqttTouched = true;
    handled = true;
  } else if (strEqualsIgnoreCase(key, "MQTT_HEARTBEAT_TOPIC")) {
    copyBounded(g_mqtt.heartbeatTopic, value);
    mqttTouched = true;
    handled = true;
  }

  if (wifiTouched) {
    markWifiUpdated();
  }
  if (mqttTouched) {
    markMqttUpdated();
  }

  return handled;
}

}  // namespace provisioning
