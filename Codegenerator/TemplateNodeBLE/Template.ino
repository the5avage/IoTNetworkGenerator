#include <BLEDevice.h>
#include "src/RemoteValues.h"
#include "src/Internal.h"
#include "CustomCode.h"

class MyClientCallback : public BLEClientCallbacks
{
    void onConnect(BLEClient *pclient)
    {
        OnConnect();
    }

    void onDisconnect(BLEClient *pclient)
    {
        connectionReady = false;
        delete device;
        device = nullptr;
        //Serial.println("onDisconnect");
        OnDisconnect();
    }
};

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    //Serial.print("BLE Advertised Device found: ");
    //Serial.println(advertisedDevice.toString().c_str());

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID))
    {
      BLEDevice::getScan()->stop();
      device = new BLEAdvertisedDevice(advertisedDevice);
    }
  }
};

/* Start connection to the BLE Server */
void connectToServer(BLEAdvertisedDevice* device)
{
    //Serial.print("Forming a connection to ");
    Serial.println(device->getAddress().toString().c_str());

    bool result = bleClient->connect(device); 
    if (!result)
    {
        //Serial.println(" - Connected to server failed");
        return;
    }

    BLERemoteService *remoteService = bleClient->getService(serviceUUID);
    if (remoteService == nullptr)
    {
        //Serial.print("Failed to find our service UUID: ");
        //Serial.println(serviceUUID.toString().c_str());
        bleClient->disconnect();
        return;
    }
    //Serial.println(" - Found our service");

    if (!loadCharacteristics(remoteService))
    {
        //Serial.print("Failed to find characteristics");
        bleClient->disconnect();
        return;
    }
    //Serial.println(" - Found all characteristic");
    connectionReady = true;
    return;
}

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
    //Serial.println("Starting Arduino BLE Client application...");
    BLEDevice::init("ESP32-BLE-Client");

    bleClient = BLEDevice::createClient();
    //Serial.println(" - Created client");

    bleClient->setClientCallbacks(new MyClientCallback());
    BLEScan* scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    scan->setInterval(1349);
    scan->setWindow(449);
    scan->setActiveScan(true);

    Setup();
    xTaskCreatePinnedToCore(taskLoop, "User Loop", 10000, nullptr, 0, nullptr, !xPortGetCoreID());
}

void loop()
{
    if (!bleClient->isConnected() && device)
    {
        //Serial.println("Connect to server");
        connectToServer(device);
    }
    else if (!bleClient->isConnected())
    {
        //Serial.println("Scan for server ...");
        BLEDevice::getScan()->start(5);
    }
    else
    {
        outputBuffer.sendData();
        delay(50);
        //Serial.println("connected to server");
    }
}