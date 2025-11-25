#include "tasks/mqtt_task.h"

#include <Arduino.h>
#include <PubSubClient.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#include <esp_system.h>
#endif

#include <string.h>

#include "config/defaults.h"
#include "config/provisioning_store.h"
#include "tasks/wifi_task.h"

namespace tasks::mqtt {
namespace {
constexpr uint32_t MQTT_RECONNECT_DELAY_MS = MQTT_RETRY_DELAY_MS;
constexpr uint32_t MQTT_HEARTBEAT_INTERVAL_MS = 5000;

provisioning::MqttInitParams g_params{};
uint32_t g_lastConfigVersion = 0;
uint32_t g_lastReconnectAttempt = 0;
uint32_t g_lastHeartbeat = 0;

WiFiClient g_netClient;
PubSubClient g_client(g_netClient);
uint8_t g_serialTxBuffer[MQTT_SERIAL_BUFFER];
size_t g_serialTxLength = 0;

uint32_t randomSuffix() {
#if defined(ESP32)
  return static_cast<uint32_t>(esp_random());
#else
  return ESP.getChipId();
#endif
}

void applyMqttServer() {
  if (!g_params.valid || g_params.host[0] == '\0') {
    return;
  }
  g_client.setServer(g_params.host, g_params.port);
}

void flushSerialBridgeBuffer() {
  if (g_serialTxLength == 0 || !g_client.connected() || g_params.publishTopic[0] == '\0') {
    g_serialTxLength = 0;
    return;
  }

  if (!g_client.publish(g_params.publishTopic, g_serialTxBuffer, g_serialTxLength)) {
    Serial.println("MQTT serial publish failed");
  }
  g_serialTxLength = 0;
}

void pumpSerialToMqtt() {
  while (Serial.available() > 0) {
    const int byteRead = Serial.read();
    if (byteRead < 0) {
      break;
    }

    if (g_serialTxLength >= sizeof(g_serialTxBuffer)) {
      flushSerialBridgeBuffer();
    }

    g_serialTxBuffer[g_serialTxLength++] = static_cast<uint8_t>(byteRead);

    if (byteRead == '\n' || byteRead == '\r') {
      flushSerialBridgeBuffer();
    }
  }
}

void mqttMessageCallback(char *topic, uint8_t *payload, unsigned int length) {
  if (g_params.commandTopic[0] != '\0' && strcmp(topic, g_params.commandTopic) == 0) {
    String message;
    message.reserve(length + 1);
    for (unsigned int i = 0; i < length; ++i) {
      message += static_cast<char>(payload[i]);
    }
    Serial.println(message);
    return;
  }

  Serial.printf("MQTT message on %s (%u bytes) ignored\n", topic, length);
}

void handleConfigUpdates() {
  const uint32_t version = provisioning::mqttVersion();
  if (version == g_lastConfigVersion) {
    return;
  }

  g_lastConfigVersion = version;
  g_params = provisioning::mqtt();
  applyMqttServer();
  if (g_client.connected()) {
    g_client.disconnect();
  }
}

bool mqttEnsureConnected() {
  if (!g_params.valid || g_params.host[0] == '\0') {
    return false;
  }

  if (g_client.connected()) {
    return true;
  }

  const uint32_t now = millis();
  if ((now - g_lastReconnectAttempt) < MQTT_RECONNECT_DELAY_MS) {
    return false;
  }
  g_lastReconnectAttempt = now;

  char clientId[48];
  const char *baseId = g_params.clientId[0] != '\0' ? g_params.clientId : "mqtt-client";
  snprintf(clientId, sizeof(clientId), "%s-%04X", baseId,
           static_cast<unsigned>(randomSuffix() & 0xFFFF));

  const char *username = g_params.username[0] == '\0' ? nullptr : g_params.username;
  const char *password = g_params.password[0] == '\0' ? nullptr : g_params.password;

  const bool connected = username == nullptr ? g_client.connect(clientId)
                                             : g_client.connect(clientId, username, password);
  if (connected) {
    Serial.println("MQTT connected");
    if (g_params.commandTopic[0] != '\0') {
      if (!g_client.subscribe(g_params.commandTopic)) {
        Serial.println("MQTT subscribe failed");
      } else {
        Serial.printf("Subscribed to %s\n", g_params.commandTopic);
      }
    }
  } else {
    Serial.printf("MQTT connect failed, rc=%d\n", g_client.state());
  }

  return connected;
}

void publishHeartbeat(uint32_t now) {
  constexpr char kHeartbeatPayload[] = "comm-robot heartbeat";
  if (g_params.publishTopic[0] != '\0') {
    if (!g_client.publish(g_params.publishTopic, kHeartbeatPayload)) {
      Serial.println("MQTT publish failed");
    }
  }

  if (g_params.heartbeatTopic[0] != '\0' &&
      strcmp(g_params.heartbeatTopic, g_params.publishTopic) != 0) {
    if (!g_client.publish(g_params.heartbeatTopic, kHeartbeatPayload)) {
      Serial.println("MQTT heartbeat publish failed");
    }
  }

  g_lastHeartbeat = now;
}

}  // namespace

void init() {
  g_client.setBufferSize(MQTT_SERIAL_BUFFER);
  g_client.setCallback(mqttMessageCallback);
  g_params = provisioning::mqtt();
  g_lastConfigVersion = provisioning::mqttVersion();
  applyMqttServer();
}

void loop() {
  handleConfigUpdates();

  if (!tasks::wifi::isConnected()) {
    if (g_client.connected()) {
      g_client.disconnect();
    }
    return;
  }

  if (!mqttEnsureConnected()) {
    return;
  }

  g_client.loop();
  pumpSerialToMqtt();

  const uint32_t now = millis();
  if ((now - g_lastHeartbeat) >= MQTT_HEARTBEAT_INTERVAL_MS) {
    publishHeartbeat(now);
  }
}

bool isConnected() { return g_client.connected(); }

}  // namespace tasks::mqtt
