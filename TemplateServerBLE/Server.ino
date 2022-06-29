#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "Characteristic.h"

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
BLEServer *server;
BLEService *service;
CODEGEN_DEFINE_CHARACTERISTICS

class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      BLEDevice::startAdvertising();
    };

    void onDisconnect(BLEServer* pServer) {
    }
};

void setup()
{
    Serial.begin(115200);

    BLEDevice::init("ESP32-BLE-Server");

    Serial.println("Starting BLE Server on Address: ");
    Serial.println(BLEDevice::getAddress().toString().c_str());
    server = BLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());
    service = server->createService(CODEGEN_SERVICE_UUID);

CODEGEN_CREATE_CHARACTERISTICS

    service->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(CODEGEN_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it in the Client!");
}

void loop()
{
    delay(2000);
}
