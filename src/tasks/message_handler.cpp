#include <Arduino.h>
#include <ESP32Servo.h>

// ----------------- Pins & Config -----------------
const int in1 = 27; // Changed from 34 (Input Only) to 27
const int in2 = 33;
const int pwm1 = 32;

const int in3 = 14;
const int in4 = 13;
const int pwm2 = 12;

const int servo1Pin = 25;
const int servo2Pin = 26;

const int PWM_CHANNEL_LEFT = 0;
const int PWM_CHANNEL_RIGHT = 1;
const int PWM_FREQ = 20000;
const int PWM_RES = 8;

// ----------------- Classes -----------------

class Motor {
private:
    int pinIn1, pinIn2, pinPwm, pwmChannel;

public:
    Motor(int in1, int in2, int pwm, int channel) 
        : pinIn1(in1), pinIn2(in2), pinPwm(pwm), pwmChannel(channel) {}

    void begin() {
        pinMode(pinIn1, OUTPUT);
        pinMode(pinIn2, OUTPUT);
        ledcSetup(pwmChannel, PWM_FREQ, PWM_RES);
        ledcAttachPin(pinPwm, pwmChannel);
    }

    void drive(const char* dir, int speed) {
        if (strcmp(dir, "forward") == 0) {
            digitalWrite(pinIn1, HIGH); digitalWrite(pinIn2, LOW);
        } else if (strcmp(dir, "backward") == 0) {
            digitalWrite(pinIn1, LOW); digitalWrite(pinIn2, HIGH);
        } else {
            digitalWrite(pinIn1, LOW); digitalWrite(pinIn2, LOW);
            speed = 0;
        }
        ledcWrite(pwmChannel, constrain(speed, 0, 255));
    }
};

class Robot {
private:
    Motor leftMotor;
    Motor rightMotor;
    Servo servo1;
    Servo servo2;
    int servo1Angle;
    bool initialized;

    // State for speed_up
    String currentLDir;
    String currentRDir;
    int currentLSpeed;
    int currentRSpeed;

public:
    Robot() : leftMotor(in1, in2, pwm1, PWM_CHANNEL_LEFT),
              rightMotor(in3, in4, pwm2, PWM_CHANNEL_RIGHT),
              servo1Angle(90), initialized(false),
              currentLDir("stop"), currentRDir("stop"),
              currentLSpeed(0), currentRSpeed(0) {}

    void begin() {
        if (initialized) return;
        
        leftMotor.begin();
        rightMotor.begin();
        
        servo1.attach(servo1Pin);
        servo1.write(servo1Angle);
        
        servo2.attach(servo2Pin);
        servo2.write(90);
        
        initialized = true;
    }

    void move(const char* ldir, const char* rdir, int lspeed, int rspeed) {
        Serial.printf("Robot Move: L=%s(%d) R=%s(%d)\n", ldir, lspeed, rdir, rspeed);
        currentLDir = ldir;
        currentRDir = rdir;
        currentLSpeed = lspeed;
        currentRSpeed = rspeed;

        leftMotor.drive(ldir, lspeed);
        rightMotor.drive(rdir, rspeed);
    }

    void adjustSpeed(int delta) {
        int newLSpeed = constrain(currentLSpeed + delta, 0, 255);
        int newRSpeed = constrain(currentRSpeed + delta, 0, 255);
        
        Serial.printf("Speed Adjust: %d -> L=%d R=%d\n", delta, newLSpeed, newRSpeed);

        // Update state and motors
        currentLSpeed = newLSpeed;
        currentRSpeed = newRSpeed;
        
        leftMotor.drive(currentLDir.c_str(), currentLSpeed);
        rightMotor.drive(currentRDir.c_str(), currentRSpeed);
    }

    void setServo1(int angle) {
        Serial.printf("Servo1: %d\n", angle);
        angle = constrain(angle, 0, 180);
        servo1Angle = angle;
        servo1.write(angle);
    }

    void spinServo2() {
        Serial.println("Servo2: Spin");
        servo2.write(180);
        delay(800);
        servo2.write(90);
        delay(100);
    }
};

// Global robot instance
Robot robot;

void onMessage(String message) {
    robot.begin(); // Ensure initialized on first message

    message.trim();
    int separatorIndex = message.indexOf(':');
    
    String cmd;
    int param = 0;

    if (separatorIndex == -1) {
        cmd = message;
    } else {
        cmd = message.substring(0, separatorIndex);
        param = message.substring(separatorIndex + 1).toInt();
    }

    if (cmd == "forward") {
        robot.move("forward", "forward", param, param);
    } else if (cmd == "backward") {
        robot.move("backward", "backward", param, param);
    } else if (cmd == "left") {
        robot.move("backward", "forward", param, param);
    } else if (cmd == "right") {
        robot.move("forward", "backward", param, param);
    } else if (cmd == "stop") {
        robot.move("stop", "stop", 0, 0);
    } else if (cmd == "speed_up") {
        robot.adjustSpeed(param);
    } else if (cmd == "servo1") {
        robot.setServo1(param);
    } else if (cmd == "servo2") {
        robot.spinServo2();
    }
}