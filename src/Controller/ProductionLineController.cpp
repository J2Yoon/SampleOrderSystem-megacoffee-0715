#include "ProductionLineController.h"

#include <algorithm>

namespace Controller
{
    ProductionLineController::ProductionLineController(
        Persistence::IProductionQueueRepository& productionQueueRepository,
        Persistence::IOrderRepository& orderRepository,
        SampleController& sampleController)
        : productionQueueRepository_(productionQueueRepository)
        , orderRepository_(orderRepository)
        , sampleController_(sampleController)
    {
    }

    std::vector<ProductionLineStatus> ProductionLineController::BuildSchedule() const
    {
        std::vector<ProductionLineStatus> schedule;

        // 단일 라인은 앞선 항목이 끝나기 전까지 다음 항목을 시작할 수 없으므로, 직전 항목의 실제
        // 완료 시각을 누적해가며 각 항목의 실제 시작/완료 시각을 계산한다(FIFO cascading schedule).
        auto previousCompletionTime = Model::ProductionQueueItem::TimePoint::min();

        for (const auto& item : productionQueueRepository_.GetAll())
        {
            const auto effectiveStartTime = std::max(item.GetProductionStartTime(), previousCompletionTime);
            const auto effectiveCompletionTime = effectiveStartTime
                + std::chrono::duration_cast<Model::ProductionQueueItem::TimePoint::duration>(
                    item.GetTotalProductionTime());

            schedule.push_back(ProductionLineStatus{ item, effectiveStartTime, effectiveCompletionTime });
            previousCompletionTime = effectiveCompletionTime;
        }
        return schedule;
    }

    void ProductionLineController::CompleteProductionQueueItem(const Model::ProductionQueueItem& item)
    {
        const auto orderOptional = orderRepository_.FindById(item.GetOrderId());
        if (orderOptional.has_value())
        {
            Model::Order order = *orderOptional;
            sampleController_.IncreaseStock(order.GetSampleId(), item.GetActualProductionQuantity());
            if (order.TryTransitionTo(Model::OrderStatus::Confirmed))
            {
                orderRepository_.Update(order);
            }
        }
        // 연결된 주문을 찾지 못하더라도(정상 흐름에서는 발생하지 않는 방어적 케이스) 큐 항목은
        // 제거해 이후 정산이 무한정 막히지 않도록 한다.
        productionQueueRepository_.Remove(item.GetOrderId());
    }

    int ProductionLineController::SettleCompletedItems(Model::ProductionQueueItem::TimePoint now)
    {
        int settledCount = 0;
        for (const auto& scheduledItem : BuildSchedule())
        {
            if (scheduledItem.effectiveCompletionTime > now)
            {
                // FIFO 누적 계산상 완료 시각은 큐 순서대로 비내림차순이므로, 아직 끝나지 않은
                // 항목을 만나면 그 뒤의 항목들도 아직 끝나지 않았음이 보장된다.
                break;
            }
            CompleteProductionQueueItem(scheduledItem.item);
            ++settledCount;
        }
        return settledCount;
    }

    std::optional<ProductionLineStatus> ProductionLineController::GetCurrentProductionStatus() const
    {
        const auto schedule = BuildSchedule();
        if (schedule.empty())
        {
            return std::nullopt;
        }
        return schedule.front();
    }

    std::vector<Model::ProductionQueueItem> ProductionLineController::GetWaitingQueue() const
    {
        auto allItems = productionQueueRepository_.GetAll();
        if (allItems.empty())
        {
            return {};
        }
        return std::vector<Model::ProductionQueueItem>(allItems.begin() + 1, allItems.end());
    }
}
