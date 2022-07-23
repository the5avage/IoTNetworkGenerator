#pragma once

#include <BLERemoteCharacteristic.h>
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
protected:
    BLERemoteCharacteristic* characteristic = nullptr;
public:
    bool get(T& value)
    {
        std::string rawValue = characteristic->readValue();
        if (rawValue.length() == sizeof(T)) {
            value = *(T*)(rawValue.data());
            return true;
        }
        return false;
    }

    RemoteValueReadOnly(BLERemoteCharacteristic* characteristic) : characteristic(characteristic) {}
    RemoteValueReadOnly() = default;
};


template<typename T>
class RemoteValue : public RemoteValueReadOnly<T>
{
public:
    void set(T value)
    {
        if (this->characteristic)
        {
            this->characteristic->writeValue((uint8_t*)&value, sizeof(T));
        }
    }

    RemoteValue(BLERemoteCharacteristic* characteristic) : RemoteValueReadOnly<T>(characteristic) {}
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
std::tuple<Args...> deserialize(uint8_t* data)
{
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

template<typename Ret, typename FN, typename P, int ...S>
Ret call_fn_internal(const FN& fn, const P& params, const seq<S...>)
{
   return fn(std::get<S>(params) ...);
}

template<typename Ret, typename ...Args>
Ret call_fn(const std::function<Ret(Args...)>& fn, 
            const std::tuple<Args...>& params)
{
    return call_fn_internal<Ret>(fn, params, typename gens<sizeof...(Args)>::type());
}

template<typename Ret, typename ...Args>
Ret call_fn(Ret(* fn)(Args...), const std::tuple<Args...>& params)
{
    return call_fn_internal<Ret>(fn, params, typename gens<sizeof...(Args)>::type());
}

template<typename FN, typename P, int ...S>
void call_fn_internal_void(const FN& fn, const P& params, const seq<S...>)
{
   return fn(std::get<S>(params) ...);
}

template<typename ...Args>
void call_fn_void(const std::function<void(Args...)>& fn, 
            const std::tuple<Args...>& params)
{
    return call_fn_internal_void(fn, params, typename gens<sizeof...(Args)>::type());
}

template<typename ...Args>
void call_fn_void(void(* fn)(Args...), const std::tuple<Args...>& params)
{
    return call_fn_internal_void(fn, params, typename gens<sizeof...(Args)>::type());
}


template <typename R, typename... Args>
class RemoteFunction
{
public:
    BLERemoteCharacteristic* characteristic = nullptr;
    SemaphoreHandle_t semaphore = xSemaphoreCreateBinary();
    nonstd::optional<R> result = nonstd::nullopt;

    nonstd::optional<R> operator()(Args... args)
    {
        result = nonstd::nullopt;
        std::vector<uint8_t> data = toBytes(args...);
        characteristic->writeValue(data.data(), data.size());
        xSemaphoreTake(semaphore, (TickType_t) 10);

        if (xSemaphoreTake(semaphore, (TickType_t) portTICK_PERIOD_MS * 5000) != pdTRUE)
        {
            return nonstd::nullopt;
        }
        return result;
    }

    void receiveResult(R value)
    {
        result = value;
        xSemaphoreGive(semaphore);
    }

    RemoteFunction() = default;
};

template <typename... Args>
class RemoteFunctionVoid
{
public:
    BLERemoteCharacteristic* characteristic = nullptr;
    SemaphoreHandle_t semaphore = xSemaphoreCreateBinary();
    bool result = false;

    bool operator()(Args... args)
    {
        result = false;
        std::vector<uint8_t> data = toBytes(args...);
        characteristic->writeValue(data.data(), data.size());
        xSemaphoreTake(semaphore, (TickType_t) 10);

        if (xSemaphoreTake(semaphore, (TickType_t) portTICK_PERIOD_MS * 5000) != pdTRUE)
        {
            false;
        }
        return result;
    }

    void receiveResult()
    {
        xSemaphoreGive(semaphore);
    }

    RemoteFunctionVoid() = default;
};

