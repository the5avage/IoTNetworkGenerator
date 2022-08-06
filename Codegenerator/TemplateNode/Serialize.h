#pragma once

#include <vector>
#include <tuple>
#include <functional>
#include <type_traits>

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
T deserialize(uint8_t*& data, T*)
{
    T result = *(T*) data;
    data += sizeof(T);
    return result;
}

template <typename... Args>
std::tuple<Args...> deserialize(std::vector<uint8_t> vec)
{
    uint8_t* data = vec.data();
    return deserialize<Args...>(data);
}

template <typename... Args>
std::tuple<Args...> deserialize(uint8_t* data)
{
    return std::tuple<Args...>{ deserialize(data, static_cast<Args*>(0))... };
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

