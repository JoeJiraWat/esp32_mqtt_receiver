#include "tasks/mqtt_task.h"

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <esp_system.h>
#include <string.h>

#include "tasks/system_bits.h"

#ifndef MQTT_HOST
#define MQTT_HOST "test.mosquitto.org"
#endif

#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

#ifndef MQTT_CLIENT_ID
#define MQTT_CLIENT_ID "esp32-comm-robot"
#endif

#ifndef MQTT_USERNAME
#define MQTT_USERNAME ""
#endif

#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD ""
#endif

#ifndef MQTT_PUB_TOPIC
#define MQTT_PUB_TOPIC "esp32/commrobot/serial_out"
#endif

#ifndef MQTT_CMD_TOPIC
#define MQTT_CMD_TOPIC "esp32/commrobot/serial_in"
#endif

#ifndef MQTT_HEARTBEAT_TOPIC
#define MQTT_HEARTBEAT_TOPIC "esp32/commrobot/heartbeat"
#endif

#ifndef MQTT_RETRY_DELAY_MS
#define MQTT_RETRY_DELAY_MS 3000
#endif

#ifndef MQTT_SERIAL_BUFFER
#define MQTT_SERIAL_BUFFER 128
#endif

namespace tasks::mqtt {
using tasks::MQTT_READY_BIT;
using tasks::WIFI_CONNECTED_BIT;
namespace {
constexpr TickType_t MQTT_RETRY_DELAY = pdMS_TO_TICKS(MQTT_RETRY_DELAY_MS);

EventGroupHandle_t g_wifiEventGroup = nullptr;
TaskHandle_t g_taskHandle = nullptr;
WiFiClient g_netClient;
PubSubClient g_client(g_netClient);
uint8_t g_serialTxBuffer[MQTT_SERIAL_BUFFER];
size_t g_serialTxLength = 0;

void flushSerialBridgeBuffer() {
  if (g_serialTxLength == 0 || !g_client.connected()) {
    g_serialTxLength = 0;
    return;
  }

  if (!g_client.publish(MQTT_PUB_TOPIC, g_serialTxBuffer, g_serialTxLength)) {
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

bool mqttEnsureConnected() {
  if (g_client.connected()) {
    return true;
  }

  char clientId[32];
  snprintf(clientId, sizeof(clientId), "%s-%04X", MQTT_CLIENT_ID,
           static_cast<unsigned>(esp_random() & 0xFFFF));

  const bool connected = g_client.connect(clientId, MQTT_USERNAME, MQTT_PASSWORD);
  if (connected) {
    Serial.println("MQTT connected");
    xEventGroupSetBits(g_wifiEventGroup, MQTT_READY_BIT);

    if (!g_client.subscribe(MQTT_CMD_TOPIC)) {
      Serial.println("MQTT subscribe failed for movement topic");
    } else {
      Serial.printf("Subscribed to %s\n", MQTT_CMD_TOPIC);
    }
  } else {
    Serial.printf("MQTT connect failed, rc=%d\n", g_client.state());
    xEventGroupClearBits(g_wifiEventGroup, MQTT_READY_BIT);
  }

  return connected;
}

void mqttMessageCallback(char *topic, uint8_t *payload, unsigned int length) {
  if (strcmp(topic, MQTT_CMD_TOPIC) == 0) {
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

void mqttTask(void * /*pvParameters*/) {
  uint32_t lastPublish = 0;

  for (;;) {
    const EventBits_t bits = xEventGroupGetBits(g_wifiEventGroup);
    if ((bits & WIFI_CONNECTED_BIT) == 0U) {
      if (g_client.connected()) {
        g_client.disconnect();
      }
      xEventGroupClearBits(g_wifiEventGroup, MQTT_READY_BIT);
      xEventGroupWaitBits(g_wifiEventGroup, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
      continue;
    }

    if (!mqttEnsureConnected()) {
      vTaskDelay(MQTT_RETRY_DELAY);
      continue;
    }

    g_client.loop();
    pumpSerialToMqtt();

    const uint32_t now = millis();
    if ((now - lastPublish) > 5000U) {
      lastPublish = now;
      constexpr char kHeartbeatPayload[] = "esp32 heartbeat";
      if (!g_client.publish(MQTT_PUB_TOPIC, kHeartbeatPayload)) {
        Serial.println("MQTT publish failed");
      }

      if (strcmp(MQTT_HEARTBEAT_TOPIC, MQTT_PUB_TOPIC) != 0) {
        if (!g_client.publish(MQTT_HEARTBEAT_TOPIC, kHeartbeatPayload)) {
          Serial.println("MQTT heartbeat publish failed");
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

}  // namespace

void init(EventGroupHandle_t wifiEventGroup) {
  configASSERT(wifiEventGroup != nullptr);
  g_wifiEventGroup = wifiEventGroup;
  g_client.setServer(MQTT_HOST, MQTT_PORT);
  g_client.setCallback(mqttMessageCallback);
}

void start(TaskHandle_t *handle, BaseType_t core) {
  configASSERT(g_wifiEventGroup != nullptr);

  if (g_taskHandle != nullptr) {
    if (handle != nullptr) {
      *handle = g_taskHandle;
    }
    return;
  }

  const BaseType_t taskCreated =
      xTaskCreatePinnedToCore(mqttTask, "mqtt", 4096, nullptr, 1, &g_taskHandle, core);
  configASSERT(taskCreated == pdPASS);

  if (handle != nullptr) {
    *handle = g_taskHandle;
  }
}

bool isConnected() { return g_client.connected(); }

}  // namespace tasks::mqtt
