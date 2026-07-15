#include "ShipmentController.h"

namespace Controller
{
    ShipmentController::ShipmentController(
        Persistence::IOrderRepository& orderRepository,
        SampleController& sampleController)
        : orderRepository_(orderRepository)
        , sampleController_(sampleController)
    {
    }

    std::vector<Model::Order> ShipmentController::GetShippableOrders() const
    {
        std::vector<Model::Order> shippableOrders;
        for (const auto& order : orderRepository_.GetAll())
        {
            if (order.GetStatus() == Model::OrderStatus::Confirmed)
            {
                shippableOrders.push_back(order);
            }
        }
        return shippableOrders;
    }

    ShipmentResult ShipmentController::ShipOrder(const std::string& orderId)
    {
        auto orderOptional = orderRepository_.FindById(orderId);
        if (!orderOptional.has_value())
        {
            return ShipmentResult::OrderNotFound;
        }

        Model::Order order = *orderOptional;
        if (order.GetStatus() != Model::OrderStatus::Confirmed)
        {
            return ShipmentResult::InvalidOrderState;
        }

        if (!sampleController_.FindSampleById(order.GetSampleId()).has_value())
        {
            return ShipmentResult::SampleNotFound;
        }

        if (!order.TryTransitionTo(Model::OrderStatus::Released))
        {
            return ShipmentResult::InvalidOrderState;
        }

        // 부분 출고는 지원하지 않는다: 주문 수량 전체를 그대로 출고 수량으로 사용한다(docs/PRD.md 4.7).
        sampleController_.DecreaseStock(order.GetSampleId(), order.GetQuantity());
        orderRepository_.Update(order);
        return ShipmentResult::Success;
    }
}
