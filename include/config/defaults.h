#pragma once

#ifndef WIFI_DEFAULT_SSID
#define WIFI_DEFAULT_SSID "kitakitan"
#endif

#ifndef WIFI_DEFAULT_PASSWORD
#define WIFI_DEFAULT_PASSWORD "bocchichan"
#endif

#ifndef MQTT_HOST
#define MQTT_HOST "10.171.48.129"
#endif

#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

#ifndef MQTT_CLIENT_ID
#define MQTT_CLIENT_ID "RobotA"
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
