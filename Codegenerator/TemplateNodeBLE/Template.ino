#include <BLEDevice.h>
#include "src/RemoteValues.h"
#include "src/Internal.h"
#include "CustomCode.h"

class MyClientCallback : public BLEClientCallbacks
{
    void onConnect(BLEClient *pclient)
    {
    }

    void onDisconnect(BLEClient *pclient)
    {
        connectionReady = false;
        delete device;
        device = nullptr;
        log("disconnected from server", Loglevel::status);
        taskBuffer.addTask([](){
            OnDisconnect();
        });
    }
};

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    log("BLE Advertised Device found: ", Loglevel::status);
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
    log("Forming a connection to " + device->getAddress().toString(), Loglevel::status);

    bool result = bleClient->connect(device);
    if (!result)
    {
        log("Connecting to device failed", Loglevel::error);
        return;
    }

    result = bleClient->setMTU(517);
    if (!result)
    {
        log("Setting MTU failed", Loglevel::error);
        return;
    }


    BLERemoteService *remoteService = bleClient->getService(serviceUUID);
    if (remoteService == nullptr)
    {
        log("Failed to get service handle", Loglevel::error);
        bleClient->disconnect();
        return;
    }

    if (!loadCharacteristics(remoteService))
    {
        log("Failed to get characteristics handles", Loglevel::error);
        bleClient->disconnect();
        return;
    }
    log("Connected to server", Loglevel::error);
    OnConnect();
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
    Setup();
    BLEDevice::init("ESP32-BLE-Client");
    BLEDevice::setMTU(517);

    bleClient = BLEDevice::createClient();

    bleClient->setClientCallbacks(new MyClientCallback());
    BLEScan* scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    scan->setInterval(1349);
    scan->setWindow(449);
    scan->setActiveScan(true);

    xTaskCreatePinnedToCore(taskLoop, "User Loop", 10000, nullptr, 0, nullptr, !xPortGetCoreID());
}

void loop()
{
    if (!bleClient->isConnected() && device)
    {
        connectToServer(device);
    }
    else if (!bleClient->isConnected())
    {
        log("Scan for server ...", Loglevel::status);
        BLEDevice::getScan()->start(5);
    }
    else
    {
        taskBuffer.executeTasks();
        delay(50);
    }
}