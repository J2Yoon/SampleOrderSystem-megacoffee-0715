#include "OrderController.h"

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Controller
{
    namespace
    {
        constexpr int kMinimumValidQuantity = 1;
        constexpr int kInitialDailySequenceNumber = 1;
        constexpr int kOrderIdSequenceDigitCount = 4;
        const std::string kOrderIdPrefix = "ORD-";
    }

    OrderController::OrderController(
        Persistence::IOrderRepository& orderRepository,
        const SampleController& sampleController)
        : orderRepository_(orderRepository)
        , sampleController_(sampleController)
    {
    }

    bool OrderController::IsValidOrderInput(const std::string& customerName, int quantity)
    {
        if (customerName.empty())
        {
            return false;
        }
        if (quantity < kMinimumValidQuantity)
        {
            return false;
        }
        return true;
    }

    std::string OrderController::FormatDateSegment(Model::Order::TimePoint timePoint)
    {
        const std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
        std::tm localTime{};
#if defined(_WIN32)
        localtime_s(&localTime, &time);
#else
        localtime_r(&time, &localTime);
#endif
        std::ostringstream dateStream;
        dateStream << std::put_time(&localTime, "%Y%m%d");
        return dateStream.str();
    }

    int OrderController::FindNextDailySequenceNumber(
        const std::vector<Model::Order>& existingOrders,
        const std::string& orderIdDatePrefix)
    {
        int maxSequenceNumberSoFar = kInitialDailySequenceNumber - 1;
        for (const auto& order : existingOrders)
        {
            const std::string& orderId = order.GetOrderId();
            if (orderId.rfind(orderIdDatePrefix, 0) != 0)
            {
                continue;
            }
            const std::string sequenceText = orderId.substr(orderIdDatePrefix.size());
            try
            {
                const int sequenceNumber = std::stoi(sequenceText);
                maxSequenceNumberSoFar = std::max(maxSequenceNumberSoFar, sequenceNumber);
            }
            catch (...)
            {
                // 예상 밖 포맷의 주문번호는 순번 계산에서 무시한다.
            }
        }
        return maxSequenceNumberSoFar + 1;
    }

    std::string OrderController::GenerateOrderId(Model::Order::TimePoint createdAt) const
    {
        const std::string orderIdDatePrefix = kOrderIdPrefix + FormatDateSegment(createdAt) + "-";
        const int nextSequenceNumber = FindNextDailySequenceNumber(orderRepository_.GetAll(), orderIdDatePrefix);

        std::ostringstream orderIdStream;
        orderIdStream << orderIdDatePrefix
            << std::setfill('0') << std::setw(kOrderIdSequenceDigitCount) << nextSequenceNumber;
        return orderIdStream.str();
    }

    OrderPlacementResult OrderController::PlaceOrder(
        const std::string& sampleId,
        const std::string& customerName,
        int quantity,
        Model::Order::TimePoint createdAt)
    {
        if (!sampleController_.IsSampleRegistered(sampleId))
        {
            return OrderPlacementResult::UnregisteredSample;
        }
        if (!IsValidOrderInput(customerName, quantity))
        {
            return OrderPlacementResult::InvalidInput;
        }

        const std::string orderId = GenerateOrderId(createdAt);
        const Model::Order newOrder(orderId, sampleId, customerName, quantity, createdAt);
        orderRepository_.Create(newOrder);
        return OrderPlacementResult::Success;
    }

    std::vector<Model::Order> OrderController::GetAllOrders() const
    {
        return orderRepository_.GetAll();
    }
}
