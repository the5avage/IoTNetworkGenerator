#include <Arduino.h>
#include "src/GenCode.h"
#include "CustomCode.h"

void Setup()
{
    Serial.begin(115200);
    Serial.println("Setup finished");
}

void Loop()
{
    if (isConnected())
    {
        auto outsideTemp = OutsideTemperatureSensor::temperature.get();
        if (outsideTemp.has_value())
        {
            Serial.print("Outside temperature: ");
            Serial.println(outsideTemp.value());
        }
        else
        {
            Serial.println("Error reading outside temperature");
        }

        auto insideTemp = InsideTemperatureSensor::temperature.get();
        if (insideTemp.has_value())
        {
            Serial.print("Inside temperature: ");
            Serial.println(insideTemp.value());
        }
        else
        {
            Serial.println("Error reading inside temperature");
        }

        auto res = OutsideTemperatureSensor::getTimeSinceStart();
        if (res)
        {
            Serial.print("OutsideTemperatureSensor timeSinceStart: ");
            Serial.println(res.value());
        }
        else
        {
            Serial.println("Error calling function OutsideTemperatureSensor::timeSinceStart");
        }
    }
    else
    {
        Serial.println("No connection to server");
    }
    delay(1000);
}

void OnConnect()
{
    Serial.println("Connected to server");
}

void OnDisconnect()
{
    Serial.println("Disconnected from server");
}

void OutsideTemperatureSensor::onChange_temperature(float value)
{
    Serial.print("Outside temp changed to: ");
    Serial.println(value);
}

void InsideTemperatureSensor::onChange_temperature(float value)
{
    Serial.print("Outside temp changed to: ");
    Serial.println(value);
}

void log(const char* message, Loglevel::Loglevel loglevel)
{
}