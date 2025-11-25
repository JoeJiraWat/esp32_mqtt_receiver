#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// ----------------- User configuration -----------------
const char* WIFI_SSID = "B4ByP1l<4¢l-lu";
const char* WIFI_PASS = "baby1234567";
const char* MQTT_BROKER = "192.168.30.193";
const int MQTT_PORT = 1883;
const char* MQTT_TOPIC = "robot/control";

// ----------------- Pins -----------------
const int in1 = 34;
const int in2 = 33;
const int pwm1 = 32;

const int in3 = 14;
const int in4 = 13;
const int pwm2 = 12;

// servo pins
const int servo1Pin = 25;
const int servo2Pin = 26;

// ----------------- PWM -----------------
const int PWM_CHANNEL_LEFT = 0;
const int PWM_CHANNEL_RIGHT = 1;
const int PWM_FREQ = 20000;
const int PWM_RES = 8;

WiFiClient espClient;
PubSubClient client(espClient);
Servo servo1;
Servo servo2;

int servo1Angle = 90;

// ----------------- WiFi Setup -----------------
void setupWiFi() {
  delay(10);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nWiFi IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi failed.");
  }
}

// ----------------- MQTT Callback -----------------
void callback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<256> doc;
  char msg[512];
  if (length >= sizeof(msg)) length = sizeof(msg)-1;
  memcpy(msg, payload, length);
  msg[length] = '\0';

  if (deserializeJson(doc, msg)) return;

  const char* cmd = doc["cmd"] | "";

  if (strcmp(cmd, "move") == 0) {
    applyDrive(doc["left"]["dir"], doc["right"]["dir"], doc["left"]["speed"], doc["right"]["speed"]);
  }

  else if (strcmp(cmd, "servo") == 0) {
    int id = doc["id"] | 0;

    if (id == 1) {
      int angle = doc["angle"] | servo1Angle;
      angle = constrain(angle, 0, 180);
      servo1Angle = angle;
      servo1.write(angle);
    }

    if (id == 2) {
      const char* action = doc["action"] | "";
      if (strcmp(action, "spin360") == 0) spinServo2Once();
    }
  }
}

// ----------------- MQTT Reconnect -----------------
void reconnect() {
  while (!client.connected()) {
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      client.subscribe(MQTT_TOPIC);
    } else {
      delay(2000);
    }
  }
}

// ----------------- Drive Motors -----------------
void applyDrive(const char* ldir, const char* rdir, int lspeed, int rspeed) {
  // Left
  if (strcmp(ldir, "forward") == 0) {
    digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
  } else if (strcmp(ldir, "backward") == 0) {
    digitalWrite(in1, LOW); digitalWrite(in2, HIGH);
  } else {
    digitalWrite(in1, LOW); digitalWrite(in2, LOW);
    lspeed = 0;
  }

  // Right
  if (strcmp(rdir, "forward") == 0) {
    digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
  } else if (strcmp(rdir, "backward") == 0) {
    digitalWrite(in3, LOW); digitalWrite(in4, HIGH);
  } else {
    digitalWrite(in3, LOW); digitalWrite(in4, LOW);
    rspeed = 0;
  }

  ledcWrite(PWM_CHANNEL_LEFT, constrain(lspeed, 0, 255));
  ledcWrite(PWM_CHANNEL_RIGHT, constrain(rspeed, 0, 255));
}

// ----------------- Servo2 spin -----------------
void spinServo2Once() {
  servo2.write(180);
  delay(800);
  servo2.write(90);
  delay(100);
}

// ----------------- SERIAL CONTROL ADDED HERE -----------------
void checkSerialControl() {
  if (!Serial.available()) return;

  char c = Serial.read();

  switch (c) {

    case 'w': case 'W':
      applyDrive("forward", "forward", 200, 200);
      Serial.println("Forward");
      break;

    case 's': case 'S':
      applyDrive("backward", "backward", 200, 200);
      Serial.println("Backward");
      break;

    case 'a': case 'A':
      applyDrive("backward", "forward", 200, 200);
      Serial.println("Left");
      break;

    case 'd': case 'D':
      applyDrive("forward", "backward", 200, 200);
      Serial.println("Right");
      break;

    case 'x': case 'X':
      applyDrive("stop", "stop", 0, 0);
      Serial.println("STOP");
      break;

    case ' ':
      Serial.println("Spin Servo2");
      spinServo2Once();
      break;

    case 'i': case 'I':
      servo1Angle = constrain(servo1Angle + 5, 0, 180);
      servo1.write(servo1Angle);
      Serial.print("Servo1: "); Serial.println(servo1Angle);
      break;

    case 'k': case 'K':
      servo1Angle = constrain(servo1Angle - 5, 0, 180);
      servo1.write(servo1Angle);
      Serial.print("Servo1: "); Serial.println(servo1Angle);
      break;
  }
}

// ----------------- setup -----------------
void setup() {
  Serial.begin(115200);

  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  ledcSetup(PWM_CHANNEL_LEFT, PWM_FREQ, PWM_RES);
  ledcAttachPin(pwm1, PWM_CHANNEL_LEFT);

  ledcSetup(PWM_CHANNEL_RIGHT, PWM_FREQ, PWM_RES);
  ledcAttachPin(pwm2, PWM_CHANNEL_RIGHT);

  servo1.attach(servo1Pin);
  servo1.write(servo1Angle);

  servo2.attach(servo2Pin);
  servo2.write(90);

  setupWiFi();
  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(callback);
}

// ----------------- loop -----------------
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  checkSerialControl();  // ← เพิ่มส่วนนี้
}

