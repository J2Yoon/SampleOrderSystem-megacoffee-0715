#include "ProductionQueueItem.h"

#include <utility>

namespace Model
{
    ProductionQueueItem::ProductionQueueItem(
        std::string orderId,
        int shortageQuantity,
        int actualProductionQuantity,
        MinutesDuration totalProductionTime,
        TimePoint productionStartTime)
        : orderId_(std::move(orderId))
        , shortageQuantity_(shortageQuantity)
        , actualProductionQuantity_(actualProductionQuantity)
        , totalProductionTime_(totalProductionTime)
        , productionStartTime_(productionStartTime)
        , expectedCompletionTime_(
            productionStartTime
            + std::chrono::duration_cast<TimePoint::duration>(totalProductionTime))
    {
    }

    const std::string& ProductionQueueItem::GetOrderId() const
    {
        return orderId_;
    }

    int ProductionQueueItem::GetShortageQuantity() const
    {
        return shortageQuantity_;
    }

    int ProductionQueueItem::GetActualProductionQuantity() const
    {
        return actualProductionQuantity_;
    }

    ProductionQueueItem::MinutesDuration ProductionQueueItem::GetTotalProductionTime() const
    {
        return totalProductionTime_;
    }

    ProductionQueueItem::TimePoint ProductionQueueItem::GetProductionStartTime() const
    {
        return productionStartTime_;
    }

    ProductionQueueItem::TimePoint ProductionQueueItem::GetExpectedCompletionTime() const
    {
        return expectedCompletionTime_;
    }
}
