#include <Arduino.h>
#include "src/GenCode.h"
#include "CustomCode.h"

std::vector<uint8_t> buffer;
std::vector<uint8_t> receivedData;
constexpr int maxCapacity = 10000;

void Setup()
{
    Serial.begin(115200);
    Serial.println("Setup finished");
}

void Loop()
{
}

void OnConnect()
{
    Serial.println("Connected to server");
}

void OnDisconnect()
{
    Serial.println("Disconnected from server");
}

static void printReceivedData()
{
    Serial.print("received Data: [");
    for (auto i: receivedData)
    {
        Serial.print(i);
        Serial.print(",");
    }
    Serial.println("]");
}

bool Receiver::inputStream(std::vector<uint8_t> data, bool isFirstPackage, bool isLastPackage)
{
    if (isFirstPackage)
    {
        buffer.clear();
    }

    if (buffer.size() + data.size() > maxCapacity)
    {
        Serial.println("Error max buffer capacity exeeded.");
        return false;
    }

    buffer.insert(buffer.end(), data.begin(), data.end());
    if (isLastPackage)
    {
        receivedData = buffer;
        printReceivedData();
    }
    return true;
}

void log(const char* message, Loglevel::Loglevel loglevel)
{
    if (loglevel >= Loglevel::error)
    {
        Serial.print("Log: ");
        Serial.println(message);
    }
}