#pragma once

#include <functional>
#include <vector>
#include "Serialize.h"
#include "optional.hpp"
#include "picosha2.h"
#include "Util.h"

template <typename T>
class ComposedAttributeAbstract
{
public:
    T cachedValue;
    bool hasValue = false;
    std::function<void(T)> onUpdate;
    int attributeSize;
    int numberOfAttributes;

    virtual void sendData(std::vector<std::vector<uint8_t>> splittedData) = 0;

    void pickUpValue(std::vector<std::vector<uint8_t>> splittedData)
    {
        std::vector<uint8_t> data;
        for (int i = 0; i < splittedData.size(); i++)
        {
            data.insert(data.begin(), splittedData[i].begin(), splittedData[i].end());
        }

        if (data.size() < 33)
        {
            log("Expected at least 33 bytes data for composed attribute", Loglevel::debug);
            return; // need at least hash value + 1 byte for valid data
        }

        std::vector<uint8_t> sendedHash(data.begin(), data.begin() + 32);
        std::vector<uint8_t> serializedData(data.begin() + 32, data.end());
        std::vector<uint8_t> calculatedHash(picosha2::k_digest_size);
        picosha2::hash256(serializedData, calculatedHash);

        for (int i = 0; i < 32; i++)
        {
            if (calculatedHash[i] != sendedHash[i])
            {
                std::string errMsg = "Hash mismatch of composed attribute. Data: " + picosha2::bytes_to_hex_string(data);
                log(errMsg.c_str(), Loglevel::error);
                return;
            }
        }

        auto deserialized = deserialize<T>(serializedData);
        if (!deserialized.has_value())
        {
            log("Cannot deserialize value of composed attribute", Loglevel::debug);
            return;
        }
        cachedValue = std::get<0>(deserialized.value());
        hasValue = true;
        if (onUpdate)
        {
            onUpdate(cachedValue);
        }
    }

    nonstd::optional<T> get()
    {
        if (hasValue)
        {
            return cachedValue;
        }
        return nonstd::nullopt;
    }

    void set(T value)
    {
        std::vector<uint8_t> serializedData = toBytes(value);
        std::vector<uint8_t> calculatedHash(picosha2::k_digest_size);
        picosha2::hash256(serializedData, calculatedHash);

        std::vector<uint8_t> data(calculatedHash.begin(), calculatedHash.end());
        data.insert(data.end(), serializedData.begin(), serializedData.end());

        std::vector<std::vector<uint8_t>> splittedData;

        auto it = data.begin();
        for (int i = 0; i < numberOfAttributes; i++)
        {
            int remaining =  data.end() - it;
            if (remaining > attributeSize)
            {
                splittedData.push_back(std::vector<uint8_t>(it, it + attributeSize));
                it += attributeSize;
            }
            else if (remaining != 0)
            {
                splittedData.push_back(std::vector<uint8_t>(it, it + remaining));
                it += remaining;
            }
            else
            {
                splittedData.push_back(std::vector<uint8_t>());
            }
        }
        sendData(splittedData);
    }
};