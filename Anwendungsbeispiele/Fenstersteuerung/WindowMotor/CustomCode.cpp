#include <Arduino.h>
#include "src/GenCode.h"
#include "CustomCode.h"
#include <ESP32Servo.h>

#define SERVO_PIN 26

Servo servoMotor;

void Setup()
{
    servoMotor.attach(SERVO_PIN);
}

void Loop()
{
}

void OnConnect()
{
}

void OnDisconnect()
{
}

bool WindowMotor::moveWindow(float targetAngle)
{
    float degree = targetAngle * 360 - 180;
    servoMotor.write(degree);
    angle.set(targetAngle);
    return true;
}

void log(const char* message, Loglevel::Loglevel loglevel)
{
}