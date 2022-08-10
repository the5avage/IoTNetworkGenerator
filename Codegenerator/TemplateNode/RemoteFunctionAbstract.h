#pragma once

#include "Serialize.h"
#include "optional.hpp"

template <typename R, typename... Args>
class RemoteFunctionAbstract
{
    SemaphoreHandle_t semaphore = xSemaphoreCreateBinary();
public:
    nonstd::optional<R> result = nonstd::nullopt;
    FunctionCallTag callTag;

    virtual void sendData(std::vector<uint8_t>& data) = 0;

    void pickUpResult(std::vector<uint8_t>& data)
    {
        auto payload = deserializeFunctionCall(data);
        if (payload)
        {
            FunctionCallData callData(payload.value());
            if (callData.callTag == callTag)
            {
                result = std::get<0>(deserialize<R>(callData.payload));
                xSemaphoreGive(semaphore);
            }
        }
    }

    nonstd::optional<R> operator()(Args... args)
    {
        callTag.rollingNumber += 1;
        result = nonstd::nullopt;
        std::vector<uint8_t> argData = toBytes(args...);
        FunctionCallData callData(callTag, argData);
        std::vector<uint8_t> data = callData.serialize();
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
    FunctionCallTag callTag;

    virtual void sendData(std::vector<uint8_t>& data) = 0;

    void pickUpResult(std::vector<uint8_t>& data)
    {
        auto payload = deserializeFunctionCall(data);
        if (payload)
        {
            FunctionCallData callData(payload.value());
            if (callData.callTag == callTag)
            {
                result = true;
                xSemaphoreGive(semaphore);
            }
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

template <typename Fun, typename... Args>
nonstd::optional<std::vector<uint8_t>> processFunctionCall(std::vector<uint8_t>& data, Fun fun)
{
    auto deserialized = deserializeFunctionCall(data);
    if (!deserialized)
    {
        return nonstd::nullopt;
    }
    FunctionCallData callData = deserialized.value();
    auto params = deserialize<Args...>(callData.payload);
    auto resultData = toBytes(call_fn(fun, params));
    auto result = FunctionCallData(callData.callTag, resultData);
    return result.serialize();
}

template <typename Fun, typename... Args>
nonstd::optional<std::vector<uint8_t>> processFunctionCallVoid(std::vector<uint8_t>& data, Fun fun)
{
    auto deserialized = deserializeFunctionCall(data);
    if (!deserialized)
    {
        return nonstd::nullopt;
    }
    FunctionCallData callData = deserialized.value();
    auto params = deserialize<Args...>(callData.payload);
    call_fn_void<Args...>(fun, params);
    std::vector<uint8_t> resultData;
    auto result = FunctionCallData(callData.callTag, resultData);
    return result.serialize();
}