#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "Characteristic.h"

BLEServer *server;
BLEService *service;
{% for node in nodes %}
namespace {{node.name}}
{
    {% for v in node.get('variables', []) %}
Characteristic<{{v.type}}> *{{v.name}};
    {% endfor %}

    {% for fun in node.get('functions', []) %}
BLECharacteristic* call_{{fun.name}};
BLECharacteristic* return_{{fun.name}};
    {% endfor %}
}
{% endfor %}

class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        BLEDevice::startAdvertising();
        Serial.println("A new client connected to BLE Server.");
    };

    void onDisconnect(BLEServer* pServer) {
    }
};

CharacteristicCallback* characteristicCallback;

void setup()
{
    Serial.begin(115200);

    BLEDevice::init("ESP32-BLE-Server");

    Serial.println("Starting BLE Server on Address: ");
    Serial.println(BLEDevice::getAddress().toString().c_str());
    server = BLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());
    service = server->createService(BLEUUID("{{service_uuid}}"), 401);

    characteristicCallback = new CharacteristicCallback();

{% for node in nodes %}
    {% for v in node.variables %}
        {% set name = node.name ~ '::' ~ v.name %}
    {{name}} = new Characteristic<{{v.type}}>(
        "{{v.uuid}}",
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
    service->addCharacteristic({{name}});
    {{name}}->setCallbacks(characteristicCallback);
        {% if v.default %}
    {{name}}->set({{v.default}});
        {% endif %}

    {% endfor %}

    {% for fun in node.functions %}
        {% set name = node.name ~ '::call_' ~ fun.name %}
    {{name}} = new BLECharacteristic(
        "{{fun.call_uuid}}",
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
    service->addCharacteristic({{name}});
    {{name}}->setCallbacks(characteristicCallback);

        {% set name = node.name ~ '::return_' ~ fun.name %}
    {{name}} = new BLECharacteristic(
        "{{fun.return_uuid}}",
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
    service->addCharacteristic({{name}});
    {{name}}->setCallbacks(characteristicCallback);
    {% endfor %}

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
