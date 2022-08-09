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
    Fun(std::get<0>(deserialize<T>(data)));
}

template<typename Fun, Fun* remoteFunction>
void notifyReturnCallback(BLERemoteCharacteristic* remoteCharacteristic, uint8_t* data, size_t length, bool isNotify)
{
    std::vector<uint8_t> vec (data, data + length);
    remoteFunction->pickUpResult(vec);
}

template<BLERemoteCharacteristic** returnCharacteristic, typename Fun, Fun* localFunction, typename... Args>
void notifyCallCallback(BLERemoteCharacteristic* remoteCharacteristic, uint8_t* data, size_t length, bool isNotify)
{
    std::vector<uint8_t> rawData(data, data + length);
    auto result = processFunctionCall<Fun*, Args...>(rawData, localFunction);
    if (!result)
    {
        return;
    }
    outputBuffer.addData(*returnCharacteristic, result.value());
}

template<BLERemoteCharacteristic** returnCharacteristic, typename Fun, Fun* localFunction, typename... Args>
void notifyCallVoidCallback(BLERemoteCharacteristic* remoteCharacteristic, uint8_t* data, size_t length, bool isNotify)
{
    std::vector<uint8_t> rawData(data, data + length);
    auto result = processFunctionCallVoid<Fun*, Args...>(rawData, localFunction);
    if (!result)
    {
        return;
    }
    outputBuffer.addData(*returnCharacteristic, result.value());
}
