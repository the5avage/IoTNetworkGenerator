#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_MCP9808.h>
#include <Adafruit_Sensor.h>
#include "CustomCode.h"
#include "src/GenCode.h"
#include "Internal.h"

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
    //Serial.begin(115200);
    setup_wifi();
    client.setServer(broker_address, broker_port);
    client.setCallback(updateValues);
    initializeValues(&client);
    Setup();
    xTaskCreatePinnedToCore(taskLoop, "User Loop", 10000, nullptr, 0, nullptr, !xPortGetCoreID());
}

void setup_wifi()
{
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void reconnect()
{
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        if (client.connect(client_name))
        {
            Serial.println("connected");
            subscribeToTopics(&client);
            if (!connectionReady)
            {
                OnConnect();
                connectionReady = true;
            }
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
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
