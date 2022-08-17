#pragma once

#include "ComposedAttributeAbstract.h"
#include "BLERemoteCharacteristic.h"

template <typename T>
class ComposedAttribute: public ComposedAttributeAbstract<T>
{
public:
    std::vector<BLERemoteCharacteristic*> characteristics;

    void sendData(std::vector<std::vector<uint8_t>> splittedData) override
    {
        assert(splittedData.size() == characteristics.size());

        for (int i = characteristics.size() - 1; i >= 0; i--)
        {
            characteristics[i]->writeValue(splittedData[i].data(), splittedData[i].size());
        }
    }

    void update()
    {
        std::vector<std::vector<uint8_t>> data;
        for (auto& c: characteristics)
        {
            std::string v = c->readValue();
            data.push_back(std::vector<uint8_t>(v.begin(), v.end()));
        }
        this->pickUpValue(data);
    }
};
