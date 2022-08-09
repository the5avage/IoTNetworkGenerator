#pragma once

#include "Serialize.h"
#include "optional.hpp"

template <typename R, typename... Args>
class RemoteFunctionAbstract
{
    SemaphoreHandle_t semaphore = xSemaphoreCreateBinary();
public:
    nonstd::optional<R> result = nonstd::nullopt;
    std::vector<uint8_t> node_uuid;

    virtual void sendData(std::vector<uint8_t>& data) = 0;

    void pickUpResult(std::vector<uint8_t>& data)
    {
        auto payload = deserializeFunctionCall(node_uuid, data);
        if (payload)
        {
            result = std::get<0>(deserialize<R>(payload.value()));
            xSemaphoreGive(semaphore);
        }
    }

    nonstd::optional<R> operator()(Args... args)
    {
        result = nonstd::nullopt;
        std::vector<uint8_t> argData = toBytes(args...);
        std::vector<uint8_t> data = serializeFunctionCall(node_uuid, argData);
        sendData(data);
        xSemaphoreTake(semaphore, (TickType_t) 10);

        if (xSemaphoreTake(semaphore, (TickType_t) portTICK_PERIOD_MS * 5000) != pdTRUE)
        {
            return nonstd::nullopt;
        }
        return result;
    }

    RemoteFunctionAbstract() = default;
};

template <typename... Args>
class RemoteFunctionVoidAbstract
{
    SemaphoreHandle_t semaphore = xSemaphoreCreateBinary();
public:
    bool result = false;
    std::vector<uint8_t> node_uuid;

    virtual void sendData(std::vector<uint8_t>& data) = 0;

    void pickUpResult(std::vector<uint8_t>& data)
    {
        auto payload = deserializeFunctionCall(node_uuid, data);
        if (payload)
        {
            result = true;
            xSemaphoreGive(semaphore);
        }
    }

    bool operator()(Args... args)
    {
        result = false;
        std::vector<uint8_t> data = toBytes(args...);
        sendData(data);
        xSemaphoreTake(semaphore, (TickType_t) 10);

        if (xSemaphoreTake(semaphore, (TickType_t) portTICK_PERIOD_MS * 5000) != pdTRUE)
        {
            false;
        }
        return result;
    }
    RemoteFunctionVoidAbstract() = default;
};
