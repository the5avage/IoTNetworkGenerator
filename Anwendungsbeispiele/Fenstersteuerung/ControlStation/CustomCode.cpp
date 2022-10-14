#include <Arduino.h>
#include "src/GenCode.h"
#include "CustomCode.h"
#include <LiquidCrystal_I2C.h>
#include <cmath>

LiquidCrystal_I2C lcd(0x27,16,2);
unsigned long outsideTempLastUpdate = 0;
bool isFirstConnect = true;

bool _automaticControl = true;
bool _targetTemperature = 23.0;

void Setup()
{
    lcd.init();
    lcd.backlight();
}

void Loop()
{
    if (isConnected())
    {
        if (isFirstConnect)
        {
            isFirstConnect = false;
            auto target = targetTemperature.get();
            if (target.has_value())
            {
                _targetTemperature = target.value();
            }
            else
            {
                targetTemperature.set(_targetTemperature);
            }

            auto automatic = automaticControl.get();
            if (automatic.has_value())
            {
                _automaticControl = automatic.value();
            }
            else
            {
                automaticControl.set(_automaticControl);
            }
        }

        lcd.setCursor(0,0);

        auto outsideTemp = OutsideTemperatureSensor::temperature.get();
        if (outsideTemp.has_value())
        {
            lcd.print("O: ");
            lcd.print(outsideTemp.value());
            lcd.print(" ");
        }
        else
        {
            lcd.print("O: Na ");
            logMessage.set("Error reading outside temperature");
        }

        auto insideTemp = InsideTemperatureSensor::temperature.get();
        if (insideTemp.has_value())
        {
            lcd.print("I: ");
            lcd.print(insideTemp.value());
        }
        else
        {
            lcd.print("I: Na");
            logMessage.set("Error reading inside temperature");
        }

        if (_automaticControl)
        {
            lcd.setCursor(0, 1);
            lcd.print("Target: ");
            lcd.print(23.0);
        }

        if (_automaticControl && insideTemp.has_value() && outsideTemp.has_value())
        {
            float in = insideTemp.value();
            float out = outsideTemp.value();
            float diff = abs(in - _targetTemperature);

            float newAngle;
            static float oldAngle = 0.0;
            if (diff > 1.0 && out < in)
            {
                newAngle = 1.0;
            }
            else if (diff < 1.0 && out > in)
            {
                newAngle = 1.0;
            }
            else
            {
                newAngle = 0.0;
            }

            if (newAngle != oldAngle)
            {
                if (newAngle > 0.5)
                    logMessage.set("Open Window.");
                else
                    logMessage.set("Close Window.");
            }

            if (!WindowMotor::moveWindow(newAngle))
            {
                logMessage.set("WindowMotor not available.");
            }
        }

        if (millis() - outsideTempLastUpdate > 60000)
        {
            logMessage.set("Outside temperature is not actual.");
        }
    }
    else
    {
        lcd.setCursor(0,0);
        lcd.print("No connection");
    }

    delay(1000);
}

void OnConnect()
{
    logMessage.set("Connected to server");
}

void OnDisconnect()
{
}

void OutsideTemperatureSensor::onChange_temperature(float value)
{
    outsideTempLastUpdate = millis();
}

void ControlStation::setTargetTemperature(float value)
{
    targetTemperature.set(value);
    _targetTemperature = value;
}

void ControlStation::setAutomaticControl(bool on)
{
    automaticControl.set(on);
    _automaticControl = on;
}

void log(const char* message, Loglevel::Loglevel loglevel)
{
    if (loglevel == Loglevel::error)
    {
        logMessage.set(message);
    }
}