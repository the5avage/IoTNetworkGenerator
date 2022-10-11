#pragma once

#include "ComposedAttributeAbstract.h"
#include "BLERemoteCharacteristic.h"
#include "Taskbuffer.h"
#include <Arduino.h>

template <typename T>
class ComposedAttribute: public ComposedAttributeAbstract<T>
{
public:
    std::vector<BLERemoteCharacteristic*> characteristics;

    void sendData(std::vector<std::vector<uint8_t>> splittedData) override
    {
        assert(splittedData.size() == characteristics.size());
        taskBuffer.addTask([=](){
            log("--> sendData", Loglevel::debug);
            for (int i = characteristics.size() - 1; i >= 0; i--)
            {
                characteristics[i]->writeValue((uint8_t*)splittedData[i].data(), splittedData[i].size());
            }
            log("<-- sendData", Loglevel::debug);
        });
    }

    void update()
    {
        log("--> update", Loglevel::debug);
        std::vector<std::vector<uint8_t>> data;
        for (int i = characteristics.size() - 1; i >= 0; i--)
        {
            std::string v = characteristics[i]->readValue();
            data.push_back(std::vector<uint8_t>(v.begin(), v.end()));
        }
        this->pickUpValue(data);
        log("<-- update", Loglevel::debug);
    }
};
