#pragma once
#include <BLEUUID.h>
#include <BLEClient.h>
#include <BLEAdvertisedDevice.h>
#include "RemoteValue.h"
#include "ComposedAttribute.h"
#include <functional>
#include "Taskbuffer.h"

extern BLEUUID serviceUUID;
extern BLEClient *bleClient;
extern BLEAdvertisedDevice* device;
extern bool connectionReady;

bool loadCharacteristics(BLERemoteService* service);

template<typename T, ComposedAttribute<T>* attribute>
void notifyComposedAttribute(BLERemoteCharacteristic* remoteCharacteristic, uint8_t* data, size_t length, bool isNotify)
{
    taskBuffer.addTask([=](){
        attribute->update();
    });
}

template<typename T, void (*Fun)(T)>
void notifyCallback(BLERemoteCharacteristic* remoteCharacteristic, uint8_t* data, size_t length, bool isNotify)
{
    taskBuffer.addTask([=](){
        std::vector<uint8_t> rawData;
        if (length >= 20)
        {
            std::string longData = remoteCharacteristic->readValue();
            rawData = std::vector<uint8_t>(longData.begin(), longData.end());
        }
        else
        {
            rawData = std::vector<uint8_t>(data, data + length);
        }
        auto deserialized = deserialize<T>(rawData);
        if (deserialized.has_value())
        {
            Fun(std::get<0>(deserialized.value()));
        }
    });
}

template<typename Fun, Fun* remoteFunction>
void notifyReturnCallback(BLERemoteCharacteristic* remoteCharacteristic, uint8_t* data, size_t length, bool isNotify)
{
    taskBuffer.addTask([=](){
        std::vector<uint8_t> rawData;
        if (length >= 20)
        {
            std::string longData = remoteCharacteristic->readValue();
            rawData = std::vector<uint8_t>(longData.begin(), longData.end());
        }
        else
        {
            rawData = std::vector<uint8_t>(data, data + length);
        }
        remoteFunction->pickUpResult(rawData);
    });
}

template<BLERemoteCharacteristic** returnCharacteristic, typename Fun, Fun* localFunction, typename... Args>
void notifyCallCallback(BLERemoteCharacteristic* remoteCharacteristic, uint8_t* data, size_t length, bool isNotify)
{
    taskBuffer.addTask([=](){
        std::vector<uint8_t> rawData;
        if (length >= 20)
        {
            std::string longData = remoteCharacteristic->readValue();
            rawData = std::vector<uint8_t>(longData.begin(), longData.end());
        }
        else
        {
            rawData = std::vector<uint8_t>(data, data + length);
        }
        auto result = processFunctionCall<Fun*, Args...>(rawData, localFunction);
        if (!result)
        {
            return;
        }
        (*returnCharacteristic)->writeValue(result.value().data(), result.value().size());
    });
}

template<BLERemoteCharacteristic** returnCharacteristic, typename Fun, Fun* localFunction, typename... Args>
void notifyCallVoidCallback(BLERemoteCharacteristic* remoteCharacteristic, uint8_t* data, size_t length, bool isNotify)
{
    taskBuffer.addTask([=](){
        std::vector<uint8_t> rawData;
        if (length >= 20)
        {
            std::string longData = remoteCharacteristic->readValue();
            rawData = std::vector<uint8_t>(longData.begin(), longData.end());
        }
        else
        {
            rawData = std::vector<uint8_t>(data, data + length);
        }
        auto result = processFunctionCallVoid<Fun*, Args...>(rawData, localFunction);
        if (!result)
        {
            return;
        }
        (*returnCharacteristic)->writeValue(result.value().data(), result.value().size());
    });
}
