#pragma once

#include <cassert>
#include <vector>
#include <tuple>
#include <functional>
#include <type_traits>
#include <string>
#include "optional.hpp"
#include <stdexcept>
#include "Util.h"

template <typename T>
std::vector<uint8_t> toBytes(T data)
{
    std::vector<uint8_t> result;
    uint8_t* tmp = (uint8_t*)&data;
    result.insert(result.begin(), tmp, tmp + sizeof(T));
    return result;
}

template <typename T>
std::vector<uint8_t> toBytes(std::vector<T> data)
{
    std::vector<uint8_t> result;
    uint32_t len = data.size();
    uint8_t* tmp = (uint8_t*)&len;
    result.insert(result.begin(), tmp, tmp + sizeof(uint32_t));

    tmp = (uint8_t*)data.data();
    for (auto& e: data)
    {
        std::vector<uint8_t> elementData = toBytes(e);
        result.insert(result.end(), elementData.begin(), elementData.end());
    }
    return result;
}

inline std::vector<uint8_t> toBytes(std::string data)
{
    std::vector<uint8_t> str(data.begin(), data.end());
    uint32_t len = str.size();
    uint8_t* tmp = (uint8_t*)&len;

    std::vector<uint8_t> result;
    result.insert(result.begin(), tmp, tmp + sizeof(uint32_t));

    result.insert(result.end(), str.begin(), str.end());
    return result;
}

inline std::vector<uint8_t> flatten(std::vector<std::vector<uint8_t>> data)
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
T deserialize(uint8_t*& data, uint8_t* end, T*)
{
    if (end - data < sizeof(T))
    {
        throw std::runtime_error("Missing bytes to deserialize value");
    }
    T result = *(T*) data;
    data += sizeof(T);
    return result;
}

template <typename T>
std::vector<T> deserialize(uint8_t*& data, uint8_t* end, std::vector<T>*)
{
    if (end - data < sizeof(uint32_t))
    {
        throw std::runtime_error("Expected at least 4 bytes containing length of vector");
    }
    std::vector<T> result;
    uint32_t len = *(uint32_t*)(data);
    data += sizeof(uint32_t);

    for (uint32_t i = 0; i < len; i++)
    {
        result.push_back(deserialize(data, end, static_cast<T*>(0)));
    }

    return result;
}

inline std::string deserialize(uint8_t*& data, uint8_t* end, std::string*)
{
    if (end - data < sizeof(uint32_t))
    {
        throw std::runtime_error("Expected at least 4 bytes containing length of string");
    }
    uint32_t len = *(uint32_t*)(data);
    data += sizeof(uint32_t);

    if (end - data < len)
    {
        throw std::runtime_error("Expected string data is missing");
    }
    std::string result(data, data + len);
    data += len;
    return result;
}

template <typename... Args>
nonstd::optional<std::tuple<Args...>> deserialize(std::vector<uint8_t> vec)
{
    uint8_t* data = vec.data();
    uint8_t* end = data + vec.size();
    try
    {
        return std::tuple<Args...>{ deserialize(data, end, static_cast<Args*>(0))... };
    }
    catch(const std::exception& e)
    {
        log(e.what(), Loglevel::error);
        return nonstd::nullopt;
    }
}

template <typename... Args>
nonstd::optional<std::tuple<Args...>> deserialize(uint8_t* data, uint8_t* end)
{
    try
    {
        return std::tuple<Args...>{ deserialize(data, end, static_cast<Args*>(0))... };
    }
    catch(const std::exception& e)
    {
        log(e.what(), Loglevel::error);
        return nonstd::nullopt;
    }
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

struct FunctionCallTag
{
    std::vector<uint8_t> calleeUUID;
    uint8_t rollingNumber;

    FunctionCallTag(
        std::vector<uint8_t> calleeUUID,
        uint8_t rollingNumber
    )
    : calleeUUID(calleeUUID), rollingNumber(rollingNumber) {
        // value provided buy Codegenerator can only be wrong due to programming error
        assert(calleeUUID.size() == 16);
    }

    FunctionCallTag(std::vector<uint8_t> rawData) {
        // must be checked before calling this constructor
        assert(rawData.size() >= 17);
        calleeUUID = std::vector<uint8_t>(rawData.begin(), rawData.begin() + 16);
        rollingNumber = rawData[16];
    }

    FunctionCallTag()
    {
        calleeUUID = std::vector<uint8_t>(16);
        rollingNumber = 0;
    }
};

inline bool operator==(FunctionCallTag& first, FunctionCallTag& second)
{
    if (first.rollingNumber != second.rollingNumber)
    {
        return false;
    }

    for (int i = 0; i < 16; i++)
    {
        if (first.calleeUUID[i] != second.calleeUUID[i])
        {
            return false;
        }
    }
    return true;
}

inline bool operator!=(FunctionCallTag& first, FunctionCallTag& second)
{
    return !(first == second);
}

struct FunctionCallData
{
    FunctionCallTag callTag;
    std::vector<uint8_t> payload;

    FunctionCallData(
        FunctionCallTag callTag,
        std::vector<uint8_t> payload
    )
    : callTag(callTag), payload(payload) {}

    std::vector<uint8_t> serialize()
    {
        std::vector<uint8_t> result(callTag.calleeUUID.begin(), callTag.calleeUUID.end());
        result.push_back(callTag.rollingNumber);
        result.insert(result.end(), payload.begin(), payload.end());
        return result;
    }
};

inline nonstd::optional< FunctionCallData > deserializeFunctionCall(std::vector<uint8_t>& data)
{
    if (data.size() < 17)
    {
        return nonstd::nullopt;
    }

    FunctionCallTag functionTag(data);

    std::vector<uint8_t> payload(data.begin() + 17, data.end());
    return FunctionCallData(functionTag, payload);
}