#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

namespace tasks::wifi {
void init();
void start(TaskHandle_t *handle = nullptr, BaseType_t core = tskNO_AFFINITY);
EventGroupHandle_t eventGroup();
}
