#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#include "config/provisioning_store.h"
#include "tasks/espnow_listener.h"
#include "tasks/mqtt_task.h"
#include "tasks/wifi_task.h"

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== Communication Robot ===");

  provisioning::initDefaults();
  tasks::wifi::init();
  tasks::mqtt::init();
  tasks::espnow::init();
}

void loop() {
  tasks::wifi::loop();
  tasks::mqtt::loop();
  tasks::espnow::loop();
  delay(10);
}