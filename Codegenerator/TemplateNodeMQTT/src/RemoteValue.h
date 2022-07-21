#pragma once
#include <PubSubClient.h>
#include <BLERemoteCharacteristic.h>
#include "Internal.h"
#include "optional.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <vector>
#include <tuple>
#include <functional>
#include <type_traits>

template<typename T>
class RemoteValueReadOnly
{
    friend void updateValues(char* topic, byte* message, unsigned int length);
protected:
    T cachedValue;
    bool hasValue = false;
public:
    bool get(T& value)
    {
        if (hasValue)
        {
            value = cachedValue;
            return true;
        }
        return false;
    }

    RemoteValueReadOnly() = default;
};

template<typename T>
class RemoteValue : public RemoteValueReadOnly<T>
{
PubSubClient* client;
const char* topic;
public:
    void set(T value)
    {
        client->publish(topic, (const uint8_t *)&value, (unsigned int) sizeof(T), true);
    }

    RemoteValue(PubSubClient* client, const char* topic) :
        RemoteValueReadOnly<T>(), client(client), topic(topic) {}
    RemoteValue() = default;
};

template <typename T>
std::vector<uint8_t> toBytes(T data)
{
    std::vector<uint8_t> result;
    uint8_t* tmp = (uint8_t*)&data;
    result.insert(result.begin(), tmp, tmp + sizeof(T));
    return result;
}

static std::vector<uint8_t> flatten(std::vector<std::vector<uint8_t>> data)
{
    std::vector<uint8_t> result;
    for (auto& v: data)
    {
        result.insert(result.end(), v.begin(), v.end());
    }
    return result;
}

template <typename... Args>
std::vector<uint8_t> toBytes(Args... args)
{
    return flatten(std::vector<std::vector<uint8_t>>{ toBytes(args)... });
}

template <typename T>
T _deserialize(uint8_t*& data)
{
    T result = *(T*) data;
    data += sizeof(T);
    return result;
}

template <typename... Args>
std::tuple<Args...> deserialize(std::vector<uint8_t> vec)
{
    uint8_t* data = vec.data();
    return std::tuple<Args...>{ _deserialize<Args>(data)... };
}

//https://riptutorial.com/cplusplus/example/24746/storing-function-arguments-in-std--tuple
template<int ...>
struct seq { };

template<int N, int ...S>
struct gens : gens<N-1, N-1, S...> { };

template<int ...S>
struct gens<0, S...> {
  typedef seq<S...> type;
};

template<typename FN, typename P, int ...S>
double call_fn_internal(const FN& fn, const P& params, const seq<S...>)
{
   return fn(std::get<S>(params) ...);
}

template<typename Ret, typename ...Args>
Ret call_fn(const std::function<Ret(Args...)>& fn, 
            const std::tuple<Args...>& params)
{
    return call_fn_internal(fn, params, typename gens<sizeof...(Args)>::type());
}

template<typename Ret, typename ...Args>
Ret call_fn(Ret(* fn)(Args...), const std::tuple<Args...>& params)
{
    return call_fn_internal(fn, params, typename gens<sizeof...(Args)>::type());
}

template <typename R, typename... Args>
class RemoteFunction
{
public:
    PubSubClient* client;
    const char* topic;
    SemaphoreHandle_t semaphore = xSemaphoreCreateBinary();
    nonstd::optional<R> result = nonstd::nullopt;
    nonstd::optional<R> operator()(Args... args)
    {
        result = nonstd::nullopt;
        std::vector<uint8_t> data = toBytes(args...);
        client->publish(topic, data.data(), data.size());
        xSemaphoreTake(semaphore, (TickType_t) 10);

        if (xSemaphoreTake(semaphore, (TickType_t) portTICK_PERIOD_MS * 5000) != pdTRUE)
        {
            return nonstd::nullopt;
        }
        return result;
    }
    RemoteFunction() = default;
    RemoteFunction(PubSubClient* client, const char* topic) :
        client(client), topic(topic) {}
};

template <typename... Args>
class RemoteFunctionVoid
{
public:
    PubSubClient* client;
    const char* topic;
    SemaphoreHandle_t semaphore = xSemaphoreCreateBinary();
    bool result = false;
    bool operator()(Args... args)
    {
        result = false;
        std::vector<uint8_t> data = toBytes(args...);
        client->publish(topic, data.data(), data.size());
        xSemaphoreTake(semaphore, (TickType_t) 10);

        if (xSemaphoreTake(semaphore, (TickType_t) portTICK_PERIOD_MS * 5000) != pdTRUE)
        {
            false;
        }
        return result;
    }
    RemoteFunctionVoid() = default;
    RemoteFunctionVoid(PubSubClient* client, const char* topic) :
        client(client), topic(topic) {}
};
