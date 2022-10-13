#include <Arduino.h>
#include "src/GenCode.h"
#include "CustomCode.h"
#include "Adafruit_MCP9808.h"

Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();

void Setup()
{
    Serial.begin(115200);
    if (!tempsensor.begin())
    {
        Serial.println("Couldn't find MCP9808!");
    }
    Serial.println("Setup finished");
}

void Loop()
{
    if (isConnected())
    {
        float currentTemp = tempsensor.readTempC();
        Serial.print("New temperature is: ");
        Serial.println(currentTemp);
        temperature.set(currentTemp);
    }
    else
    {
        Serial.println("No connection to server");
    }
    delay(10000);
}

void OnConnect()
{
    Serial.println("Connected to server");
    tolerance.set(0.1);
}

void OnDisconnect()
{
    Serial.println("Disconnected from server");
}

void log(const char* message, Loglevel::Loglevel loglevel)
{
}