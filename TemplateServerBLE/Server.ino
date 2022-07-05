#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "Characteristic.h"

BLEServer *server;
BLEService *service;
{% for v in values %}
Characteristic<{{v.type}}> *{{v.name}};
{% endfor %}

class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        BLEDevice::startAdvertising();
        Serial.println("A new client connected to BLE Server.");
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
    service = server->createService("{{service_uuid}}");

{% for v in values %}
    {{v.name}} = new Characteristic<{{v.type}}>(
        "{{v.uuid}}",
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
    service->addCharacteristic({{v.name}});
    {% if v.default %}
    {{v.name}}->set({{v.default}});
    {% endif %}

{% endfor %}
    service->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID("{{service_uuid}}");
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
