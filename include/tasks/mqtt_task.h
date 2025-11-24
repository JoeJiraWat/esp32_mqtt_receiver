#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

namespace tasks::mqtt {
void init(EventGroupHandle_t wifiEventGroup);
void start(TaskHandle_t *handle = nullptr, BaseType_t core = tskNO_AFFINITY);
bool isConnected();
}
