#pragma once

namespace tasks::wifi {
void init();
void loop();
bool isConnected();
void requestReconnect();
}
