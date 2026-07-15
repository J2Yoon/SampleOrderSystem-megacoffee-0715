#include "Order.h"

#include <utility>

namespace Model
{
    Order::Order(
        std::string orderId,
        std::string sampleId,
        std::string customerName,
        int quantity,
        TimePoint createdAt)
        : Order(
            std::move(orderId),
            std::move(sampleId),
            std::move(customerName),
            quantity,
            OrderStatus::Reserved,
            createdAt)
    {
    }

    Order::Order(
        std::string orderId,
        std::string sampleId,
        std::string customerName,
        int quantity,
        OrderStatus status,
        TimePoint createdAt)
        : orderId_(std::move(orderId))
        , sampleId_(std::move(sampleId))
        , customerName_(std::move(customerName))
        , quantity_(quantity)
        , status_(status)
        , createdAt_(createdAt)
    {
    }

    const std::string& Order::GetOrderId() const
    {
        return orderId_;
    }

    const std::string& Order::GetSampleId() const
    {
        return sampleId_;
    }

    const std::string& Order::GetCustomerName() const
    {
        return customerName_;
    }

    int Order::GetQuantity() const
    {
        return quantity_;
    }

    OrderStatus Order::GetStatus() const
    {
        return status_;
    }

    Order::TimePoint Order::GetCreatedAt() const
    {
        return createdAt_;
    }

    bool Order::TryTransitionTo(OrderStatus newStatus)
    {
        if (!IsValidOrderStatusTransition(status_, newStatus))
        {
            return false;
        }
        status_ = newStatus;
        return true;
    }
}
