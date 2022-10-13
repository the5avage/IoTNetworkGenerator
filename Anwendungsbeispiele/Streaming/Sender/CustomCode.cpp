#include <Arduino.h>
#include "src/GenCode.h"
#include "CustomCode.h"

void Setup()
{
    Serial.begin(115200);
    Serial.println("Setup finished");
}

constexpr int chunkSize = 50;

std::vector<uint8_t> testdata;
uint8_t count = 0;

static void sendTestData()
{
    Serial.println("Send testdata start");
    bool isFirstPackage = true;
    bool isLastPackage = false;
    for (int i = 0; i < testdata.size(); i += chunkSize)
    {
        int remaining =  testdata.size() - i;
        int thisChunkSize;
        if (remaining > chunkSize)
        {
            thisChunkSize = chunkSize;
        }
        else
        {
            thisChunkSize = remaining;
            isLastPackage = true;
        }
        std::vector<uint8_t> chunk(testdata.begin() + i, testdata.begin() + i + thisChunkSize);
        auto success = Receiver::inputStream(chunk, isFirstPackage, isLastPackage);
        if (!success.has_value() || !success.value())
        {
            Serial.print("Send package failed");
            break;
        }
        isFirstPackage = false;
        delay(100);
    }
    Serial.println("Send testdata finished");
}

void Loop()
{
    if (isConnected())
    {
        for (int i = 0; i < 100; i++)
        {
            testdata.push_back(count);
        }
        count++;
        sendTestData();
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

void log(const char* message, Loglevel::Loglevel loglevel)
{
    if (loglevel >= Loglevel::debug)
    {
        Serial.print("Log: ");
        Serial.println(message);
    }
}