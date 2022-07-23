#pragma once
#include <BLEUUID.h>
#include <BLEClient.h>
#include <BLEAdvertisedDevice.h>
#include "RemoteValue.h"

extern BLEUUID serviceUUID;
extern BLEClient *bleClient;
extern BLEAdvertisedDevice* device;
extern bool connectionReady;

bool loadCharacteristics(BLERemoteService* service);

struct OutputBufferEntry
{
    BLERemoteCharacteristic* target;
    std::vector<uint8_t> data;
};

struct OutputBuffer
{
    SemaphoreHandle_t semaphore;
    std::vector<OutputBufferEntry> buffer;

    void addData(BLERemoteCharacteristic* target, std::vector<uint8_t>& data);
    void sendData();

    OutputBuffer()
    {
        semaphore = xSemaphoreCreateBinary();
        xSemaphoreGive(semaphore);
    }
};

extern OutputBuffer outputBuffer;

template<typename T, void (*Fun)(T)>
void notifyCallback(BLERemoteCharacteristic* remoteCharacteristic, uint8_t* data, size_t length, bool isNotify)
{
    if (length == sizeof(T))
    {
        Fun(*(T*)data);
    }
}

template<typename T, typename Fun, Fun* remoteFunction>
void notifyReturnCallback(BLERemoteCharacteristic* remoteCharacteristic, uint8_t* data, size_t length, bool isNotify)
{
    if (length == sizeof(T))
    {
        remoteFunction->receiveResult(*(T*)data);
    }
}

template<typename Fun, Fun* remoteFunction>
void notifyReturnVoidCallback(BLERemoteCharacteristic* remoteCharacteristic, uint8_t* data, size_t length, bool isNotify)
{
    remoteFunction->receiveResult();
}

template<BLERemoteCharacteristic** returnCharacteristic, typename Fun, Fun* localFunction, typename... Args>
void notifyCallCallback(BLERemoteCharacteristic* remoteCharacteristic, uint8_t* data, size_t length, bool isNotify)
{
    auto params = deserialize<Args...>(data);
    auto result = toBytes(call_fn(localFunction, params));
    outputBuffer.addData(*returnCharacteristic, result);
}

template<BLERemoteCharacteristic** returnCharacteristic, typename Fun, Fun* localFunction, typename... Args>
void notifyCallVoidCallback(BLERemoteCharacteristic* remoteCharacteristic, uint8_t* data, size_t length, bool isNotify)
{
    auto params = deserialize<Args...>(data);
    call_fn_void<Args...>(localFunction, params);
    std::vector<uint8_t> result;
    outputBuffer.addData(*returnCharacteristic, result);
}
