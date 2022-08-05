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
        float innenTemp;
        float aussenTemp;
        if (InsideTemperatureSensor::temperature.get(innenTemp))
        {
            Serial.print("Innentemperatur: ");
            Serial.print(innenTemp);
        }
        else
        {
            Serial.println("Error reading inside temperature");
        }

        if (OutsideTemperatureSensor::temperature.get(aussenTemp))
        {
            Serial.print(" Aussentemperatur: ");
            Serial.println(aussenTemp);
        }
        else
        {
            Serial.println("Error reading outside temperature");
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
