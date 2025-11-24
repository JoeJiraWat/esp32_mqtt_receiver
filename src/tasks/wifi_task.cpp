#include "tasks/wifi_task.h"

#include <Arduino.h>
#include <WiFi.h>

#include "tasks/system_bits.h"

namespace tasks::wifi {
using tasks::MQTT_READY_BIT;
using tasks::WIFI_CONNECTED_BIT;
using tasks::WIFI_FAIL_BIT;
namespace {
constexpr char WIFI_SSID[] = "Wokwi-GUEST";
constexpr char WIFI_PASSWORD[] = "";
constexpr uint8_t WIFI_MAX_RETRIES = 5;
constexpr TickType_t WIFI_RETRY_DELAY = pdMS_TO_TICKS(5000);

EventGroupHandle_t g_eventGroup = nullptr;
TaskHandle_t g_taskHandle = nullptr;
uint8_t g_retryCount = 0;

void handleWiFiEvent(WiFiEvent_t event) {
  if (g_eventGroup == nullptr) {
    return;
  }

  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_START:
      Serial.println("WiFi interface started");
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("WiFi linked with AP");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.printf("IP acquired: %s\n", WiFi.localIP().toString().c_str());
      g_retryCount = 0;
      xEventGroupClearBits(g_eventGroup, WIFI_FAIL_BIT);
      xEventGroupSetBits(g_eventGroup, WIFI_CONNECTED_BIT);
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WiFi disconnected; scheduling retry");
      xEventGroupClearBits(g_eventGroup, WIFI_CONNECTED_BIT | MQTT_READY_BIT);
      xEventGroupSetBits(g_eventGroup, WIFI_FAIL_BIT);
      break;
    default:
      break;
  }
}

void wifiConnectTask(void * /*pvParameters*/) {
  WiFi.mode(WIFI_STA);

  for (;;) {
    if (WiFi.status() != WL_CONNECTED && g_retryCount < WIFI_MAX_RETRIES) {
      Serial.printf("Connecting to %s (attempt %u/%u)\n", WIFI_SSID, g_retryCount + 1,
                    WIFI_MAX_RETRIES);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      ++g_retryCount;
    }

    const EventBits_t bits =
        xEventGroupWaitBits(g_eventGroup, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE,
                            pdMS_TO_TICKS(1000));

    if ((bits & WIFI_CONNECTED_BIT) != 0U) {
      // Serial.println("WiFi link verified by task");
      vTaskDelay(pdMS_TO_TICKS(10000));
    } else if ((bits & WIFI_FAIL_BIT) != 0U) {
      if (g_retryCount >= WIFI_MAX_RETRIES) {
        Serial.println("Max WiFi retries reached; backing off");
        g_retryCount = 0;
        vTaskDelay(WIFI_RETRY_DELAY);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

}  // namespace

EventGroupHandle_t eventGroup() { return g_eventGroup; }

void init() {
  if (g_eventGroup == nullptr) {
    g_eventGroup = xEventGroupCreate();
    configASSERT(g_eventGroup != nullptr);
  }

  WiFi.disconnect(true, true);
  WiFi.onEvent(handleWiFiEvent);
}

void start(TaskHandle_t *handle, BaseType_t core) {
  configASSERT(g_eventGroup != nullptr);

  if (g_taskHandle != nullptr) {
    if (handle != nullptr) {
      *handle = g_taskHandle;
    }
    return;
  }

  const BaseType_t taskCreated = xTaskCreatePinnedToCore(
      wifiConnectTask, "wifi-connect", 4096, nullptr, 1, &g_taskHandle, core);
  configASSERT(taskCreated == pdPASS);

  if (handle != nullptr) {
    *handle = g_taskHandle;
  }
}

}  // namespace tasks::wifi
