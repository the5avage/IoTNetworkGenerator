#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_MCP9808.h>
#include <Adafruit_Sensor.h>
#include "CustomCode.h"
#include "src/GenCode.h"
#include "Internal.h"
#include "src/Util.h"

WiFiClient espClient;
PubSubClient client(espClient);

static void taskLoop(void* param)
{
    for (;;)
    {
        Loop();
    }
}

void setup()
{
    Setup();
    setup_wifi();
    client.setServer(broker_address, broker_port);
    client.setCallback(updateValues);
    initializeValues(&client);
    xTaskCreatePinnedToCore(taskLoop, "User Loop", 10000, nullptr, 0, nullptr, !xPortGetCoreID());
}

void setup_wifi()
{
    delay(10);
    // We start by connecting to a WiFi network
    log("Connecting to Wifi", Loglevel::status);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    log("Wifi connected", Loglevel::status);
}

void reconnect()
{
    while (!client.connected())
    {
        log("Attemting mqtt connection", Loglevel::status);
        if (client.connect(client_name))
        {
            subscribeToTopics(&client);
            if (!connectionReady)
            {
                log("connected to server", Loglevel::status);
                OnConnect();
                connectionReady = true;
            }
        }
        else
        {
            log("connection to server failed", Loglevel::error);
            delay(5000);
        }
    }
}

void loop()
{
    if (!client.connected())
    {
        if (connectionReady)
        {
            OnDisconnect();
            connectionReady = false;
        }
        reconnect();
    }
    client.loop();
}
