#include "OrderApprovalController.h"

#include <cmath>

namespace Controller
{
    OrderApprovalController::OrderApprovalController(
        Persistence::IOrderRepository& orderRepository,
        const SampleController& sampleController,
        Persistence::IProductionQueueRepository& productionQueueRepository)
        : orderRepository_(orderRepository)
        , sampleController_(sampleController)
        , productionQueueRepository_(productionQueueRepository)
    {
    }

    std::vector<Model::Order> OrderApprovalController::GetReservedOrders() const
    {
        std::vector<Model::Order> reservedOrders;
        for (const auto& order : orderRepository_.GetAll())
        {
            if (order.GetStatus() == Model::OrderStatus::Reserved)
            {
                reservedOrders.push_back(order);
            }
        }
        return reservedOrders;
    }

    int OrderApprovalController::CalculateActualProductionQuantity(int shortageQuantity, double yield)
    {
        return static_cast<int>(std::ceil(static_cast<double>(shortageQuantity) / yield));
    }

    Model::ProductionQueueItem::MinutesDuration OrderApprovalController::CalculateTotalProductionTime(
        double averageProductionMinutesPerUnit, int actualProductionQuantity)
    {
        return Model::ProductionQueueItem::MinutesDuration(
            averageProductionMinutesPerUnit * actualProductionQuantity);
    }

    Model::ProductionQueueItem OrderApprovalController::CreateProductionQueueItem(
        const Model::Order& order,
        const Model::Sample& sample,
        int shortageQuantity,
        Model::Order::TimePoint approvedAt)
    {
        const int actualProductionQuantity =
            CalculateActualProductionQuantity(shortageQuantity, sample.GetYield());
        const auto totalProductionTime =
            CalculateTotalProductionTime(sample.GetAverageProductionMinutesPerUnit(), actualProductionQuantity);

        return Model::ProductionQueueItem(
            order.GetOrderId(),
            shortageQuantity,
            actualProductionQuantity,
            totalProductionTime,
            approvedAt);
    }

    OrderApprovalResult OrderApprovalController::ApproveOrder(
        const std::string& orderId,
        Model::Order::TimePoint approvedAt)
    {
        auto orderOptional = orderRepository_.FindById(orderId);
        if (!orderOptional.has_value())
        {
            return OrderApprovalResult::OrderNotFound;
        }

        Model::Order order = *orderOptional;
        if (order.GetStatus() != Model::OrderStatus::Reserved)
        {
            return OrderApprovalResult::InvalidOrderState;
        }

        const auto sampleOptional = sampleController_.FindSampleById(order.GetSampleId());
        if (!sampleOptional.has_value())
        {
            return OrderApprovalResult::SampleNotFound;
        }

        // 승인 시점 재고는 조회만 하며 물리적으로 차감하지 않는다(docs/PRD.md 4.6.1).
        const int shortageQuantity = order.GetQuantity() - sampleOptional->GetStock();

        if (shortageQuantity <= 0)
        {
            order.TryTransitionTo(Model::OrderStatus::Confirmed);
        }
        else
        {
            const Model::ProductionQueueItem productionQueueItem =
                CreateProductionQueueItem(order, *sampleOptional, shortageQuantity, approvedAt);
            productionQueueRepository_.Create(productionQueueItem);
            order.TryTransitionTo(Model::OrderStatus::Producing);
        }

        orderRepository_.Update(order);
        return OrderApprovalResult::Success;
    }

    OrderRejectionResult OrderApprovalController::RejectOrder(const std::string& orderId)
    {
        auto orderOptional = orderRepository_.FindById(orderId);
        if (!orderOptional.has_value())
        {
            return OrderRejectionResult::OrderNotFound;
        }

        Model::Order order = *orderOptional;
        if (!order.TryTransitionTo(Model::OrderStatus::Rejected))
        {
            return OrderRejectionResult::InvalidOrderState;
        }

        orderRepository_.Update(order);
        return OrderRejectionResult::Success;
    }
}
