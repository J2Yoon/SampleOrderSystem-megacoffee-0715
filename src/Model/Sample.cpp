#include "Sample.h"

#include <utility>

namespace Model
{
    Sample::Sample(
        std::string sampleId,
        std::string name,
        double averageProductionMinutesPerUnit,
        double yield,
        int stock)
        : sampleId_(std::move(sampleId))
        , name_(std::move(name))
        , averageProductionMinutesPerUnit_(averageProductionMinutesPerUnit)
        , yield_(yield)
        , stock_(stock)
    {
    }

    const std::string& Sample::GetSampleId() const
    {
        return sampleId_;
    }

    const std::string& Sample::GetName() const
    {
        return name_;
    }

    double Sample::GetAverageProductionMinutesPerUnit() const
    {
        return averageProductionMinutesPerUnit_;
    }

    double Sample::GetYield() const
    {
        return yield_;
    }

    int Sample::GetStock() const
    {
        return stock_;
    }

    void Sample::SetStock(int stock)
    {
        stock_ = stock;
    }
}
