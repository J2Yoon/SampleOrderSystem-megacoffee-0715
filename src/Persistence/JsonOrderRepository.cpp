#include "JsonOrderRepository.h"

#include "JsonRepositoryUtil.h"

namespace Persistence
{
    namespace
    {
        std::string GetOrderKey(const Model::Order& order)
        {
            return order.GetOrderId();
        }
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
        json.Set("status", Json::Value::MakeString(Model::OrderStatusToString(order.GetStatus())));
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
            Model::OrderStatusFromString(json.GetString("status")),
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
