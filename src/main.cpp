#include <Arduino.h>
#include <WiFi.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tasks/mqtt_task.h"
#include "tasks/wifi_task.h"

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== ESP32 FreeRTOS WiFi ===");

  tasks::wifi::init();
  tasks::wifi::start();

  tasks::mqtt::init(tasks::wifi::eventGroup());
  tasks::mqtt::start();
}

void loop() {
  static uint32_t lastHeartbeat = 0;
  const uint32_t now = millis();

  if ((now - lastHeartbeat) > 2000U) {
    lastHeartbeat = now;
    // Serial.printf("Loop alive | WiFi status: %d | MQTT: %s\n", WiFi.status(),
                  // tasks::mqtt::isConnected() ? "connected" : "idle");
  }

  vTaskDelay(pdMS_TO_TICKS(10));
}