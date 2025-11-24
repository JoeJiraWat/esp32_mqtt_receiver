#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

namespace tasks {
constexpr EventBits_t WIFI_CONNECTED_BIT = BIT0;
constexpr EventBits_t WIFI_FAIL_BIT = BIT1;
constexpr EventBits_t MQTT_READY_BIT = BIT2;
}  // namespace tasks
