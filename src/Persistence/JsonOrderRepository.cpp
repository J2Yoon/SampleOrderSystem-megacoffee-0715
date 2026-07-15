#include "JsonOrderRepository.h"

#include <array>
#include <stdexcept>
#include <utility>

#include "JsonRepositoryUtil.h"

namespace Persistence
{
    namespace
    {
        std::string GetOrderKey(const Model::Order& order)
        {
            return order.GetOrderId();
        }

        using OrderStatusNamePair = std::pair<Model::OrderStatus, const char*>;

        // OrderStatus <-> 문자열 매핑 테이블. Model::OrderStatus.cpp의 상태 전이 테이블과 동일하게
        // 선언형 테이블 방식으로 관리해 상태값 추가 시 실수 여지를 줄인다.
        constexpr std::array<OrderStatusNamePair, 5> kOrderStatusNames
        {
            OrderStatusNamePair{ Model::OrderStatus::Reserved,  "RESERVED" },
            OrderStatusNamePair{ Model::OrderStatus::Rejected,  "REJECTED" },
            OrderStatusNamePair{ Model::OrderStatus::Producing, "PRODUCING" },
            OrderStatusNamePair{ Model::OrderStatus::Confirmed, "CONFIRMED" },
            OrderStatusNamePair{ Model::OrderStatus::Released,  "RELEASED" },
        };
    }

    JsonOrderRepository::JsonOrderRepository(std::string filePath)
        : filePath_(std::move(filePath))
    {
        Load();
    }

    void JsonOrderRepository::Load()
    {
        orders_ = JsonRepositoryUtil::LoadEntitiesFromFile<Model::Order>(filePath_, &FromJson);
    }

    void JsonOrderRepository::Persist() const
    {
        JsonRepositoryUtil::PersistEntitiesToFile(filePath_, orders_, &ToJson);
    }

    std::string JsonOrderRepository::OrderStatusToString(Model::OrderStatus status)
    {
        for (const auto& entry : kOrderStatusNames)
        {
            if (entry.first == status)
            {
                return entry.second;
            }
        }
        throw std::invalid_argument("알 수 없는 OrderStatus 값입니다.");
    }

    Model::OrderStatus JsonOrderRepository::OrderStatusFromString(const std::string& statusText)
    {
        for (const auto& entry : kOrderStatusNames)
        {
            if (statusText == entry.second)
            {
                return entry.first;
            }
        }
        return Model::OrderStatus::Reserved; // 알 수 없는 값은 안전하게 초기 상태로 폴백한다.
    }

    long long JsonOrderRepository::ToEpochMilliseconds(Model::Order::TimePoint timePoint)
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch()).count();
    }

    Model::Order::TimePoint JsonOrderRepository::FromEpochMilliseconds(long long epochMilliseconds)
    {
        return Model::Order::TimePoint(std::chrono::milliseconds(epochMilliseconds));
    }

    Json::Value JsonOrderRepository::ToJson(const Model::Order& order)
    {
        Json::Value json = Json::Value::MakeObject();
        json.Set("orderId", Json::Value::MakeString(order.GetOrderId()));
        json.Set("sampleId", Json::Value::MakeString(order.GetSampleId()));
        json.Set("customerName", Json::Value::MakeString(order.GetCustomerName()));
        json.Set("quantity", Json::Value::MakeNumber(order.GetQuantity()));
        json.Set("status", Json::Value::MakeString(OrderStatusToString(order.GetStatus())));
        json.Set("createdAtEpochMillis", Json::Value::MakeNumber(static_cast<double>(ToEpochMilliseconds(order.GetCreatedAt()))));
        return json;
    }

    Model::Order JsonOrderRepository::FromJson(const Json::Value& json)
    {
        return Model::Order(
            json.GetString("orderId"),
            json.GetString("sampleId"),
            json.GetString("customerName"),
            json.GetInt("quantity"),
            OrderStatusFromString(json.GetString("status")),
            FromEpochMilliseconds(json.GetInt64("createdAtEpochMillis")));
    }

    bool JsonOrderRepository::Create(const Model::Order& order)
    {
        if (FindById(order.GetOrderId()).has_value())
        {
            return false; // 이미 등록된 주문번호
        }

        orders_.push_back(order);
        Persist();
        return true;
    }

    std::vector<Model::Order> JsonOrderRepository::GetAll() const
    {
        return orders_;
    }

    std::optional<Model::Order> JsonOrderRepository::FindById(const std::string& orderId) const
    {
        const auto found = JsonRepositoryUtil::FindIteratorByKey(orders_, orderId, &GetOrderKey);

        if (found == orders_.end())
        {
            return std::nullopt;
        }
        return *found;
    }

    bool JsonOrderRepository::Update(const Model::Order& order)
    {
        const auto found = JsonRepositoryUtil::FindIteratorByKey(orders_, GetOrderKey(order), &GetOrderKey);

        if (found == orders_.end())
        {
            return false;
        }

        *found = order;
        Persist();
        return true;
    }

    bool JsonOrderRepository::Remove(const std::string& orderId)
    {
        const auto found = JsonRepositoryUtil::FindIteratorByKey(orders_, orderId, &GetOrderKey);

        if (found == orders_.end())
        {
            return false;
        }

        orders_.erase(found);
        Persist();
        return true;
    }
}
