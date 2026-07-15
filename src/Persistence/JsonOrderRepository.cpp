#include "JsonOrderRepository.h"

#include <algorithm>
#include <stdexcept>

#include "../Json/JsonIO.h"

namespace Persistence
{
    JsonOrderRepository::JsonOrderRepository(std::string filePath)
        : filePath_(std::move(filePath))
    {
        Load();
    }

    void JsonOrderRepository::Load()
    {
        orders_.clear();

        const auto fileContents = Json::FileIO::ReadAllText(filePath_);
        if (!fileContents.has_value())
        {
            return; // 파일이 없으면 빈 목록으로 시작한다(최초 실행 등).
        }

        Json::Value root;
        if (!Json::Value::Parse(*fileContents, root) || !root.IsArray())
        {
            return; // 파싱 실패 시에도 예외 없이 빈 목록으로 폴백한다.
        }

        for (const auto& item : root.Items())
        {
            orders_.push_back(FromJson(item));
        }
    }

    void JsonOrderRepository::Persist() const
    {
        Json::Value root = Json::Value::MakeArray();
        for (const auto& order : orders_)
        {
            root.Add(ToJson(order));
        }
        Json::FileIO::WriteAllText(filePath_, root.Dump());
    }

    std::string JsonOrderRepository::OrderStatusToString(Model::OrderStatus status)
    {
        switch (status)
        {
        case Model::OrderStatus::Reserved:  return "RESERVED";
        case Model::OrderStatus::Rejected:  return "REJECTED";
        case Model::OrderStatus::Producing: return "PRODUCING";
        case Model::OrderStatus::Confirmed: return "CONFIRMED";
        case Model::OrderStatus::Released:  return "RELEASED";
        }
        throw std::invalid_argument("알 수 없는 OrderStatus 값입니다.");
    }

    Model::OrderStatus JsonOrderRepository::OrderStatusFromString(const std::string& statusText)
    {
        if (statusText == "RESERVED")  return Model::OrderStatus::Reserved;
        if (statusText == "REJECTED")  return Model::OrderStatus::Rejected;
        if (statusText == "PRODUCING") return Model::OrderStatus::Producing;
        if (statusText == "CONFIRMED") return Model::OrderStatus::Confirmed;
        if (statusText == "RELEASED")  return Model::OrderStatus::Released;
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
        const auto found = std::find_if(orders_.begin(), orders_.end(),
            [&orderId](const Model::Order& order) { return order.GetOrderId() == orderId; });

        if (found == orders_.end())
        {
            return std::nullopt;
        }
        return *found;
    }

    bool JsonOrderRepository::Update(const Model::Order& order)
    {
        const auto found = std::find_if(orders_.begin(), orders_.end(),
            [&order](const Model::Order& existing) { return existing.GetOrderId() == order.GetOrderId(); });

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
        const auto found = std::find_if(orders_.begin(), orders_.end(),
            [&orderId](const Model::Order& order) { return order.GetOrderId() == orderId; });

        if (found == orders_.end())
        {
            return false;
        }

        orders_.erase(found);
        Persist();
        return true;
    }
}
