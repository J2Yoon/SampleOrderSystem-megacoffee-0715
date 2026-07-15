#include "JsonProductionQueueRepository.h"

#include <algorithm>

#include "../Json/JsonIO.h"

namespace Persistence
{
    JsonProductionQueueRepository::JsonProductionQueueRepository(std::string filePath)
        : filePath_(std::move(filePath))
    {
        Load();
    }

    void JsonProductionQueueRepository::Load()
    {
        items_.clear();

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
            items_.push_back(FromJson(item));
        }
    }

    void JsonProductionQueueRepository::Persist() const
    {
        Json::Value root = Json::Value::MakeArray();
        for (const auto& item : items_)
        {
            root.Add(ToJson(item));
        }
        Json::FileIO::WriteAllText(filePath_, root.Dump());
    }

    long long JsonProductionQueueRepository::ToEpochMilliseconds(Model::ProductionQueueItem::TimePoint timePoint)
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch()).count();
    }

    Model::ProductionQueueItem::TimePoint JsonProductionQueueRepository::FromEpochMilliseconds(long long epochMilliseconds)
    {
        return Model::ProductionQueueItem::TimePoint(std::chrono::milliseconds(epochMilliseconds));
    }

    Json::Value JsonProductionQueueRepository::ToJson(const Model::ProductionQueueItem& item)
    {
        Json::Value json = Json::Value::MakeObject();
        json.Set("orderId", Json::Value::MakeString(item.GetOrderId()));
        json.Set("shortageQuantity", Json::Value::MakeNumber(item.GetShortageQuantity()));
        json.Set("actualProductionQuantity", Json::Value::MakeNumber(item.GetActualProductionQuantity()));
        json.Set("totalProductionMinutes", Json::Value::MakeNumber(item.GetTotalProductionTime().count()));
        json.Set("productionStartEpochMillis",
            Json::Value::MakeNumber(static_cast<double>(ToEpochMilliseconds(item.GetProductionStartTime()))));
        // 예상 완료 시각은 생성자에서 항상 재계산되는 파생값이지만, 파일만 보고도 완료 예정 시점을
        // 바로 파악할 수 있도록 참고용으로 함께 기록한다(로드 시에는 사용하지 않음).
        json.Set("expectedCompletionEpochMillis",
            Json::Value::MakeNumber(static_cast<double>(ToEpochMilliseconds(item.GetExpectedCompletionTime()))));
        return json;
    }

    Model::ProductionQueueItem JsonProductionQueueRepository::FromJson(const Json::Value& json)
    {
        return Model::ProductionQueueItem(
            json.GetString("orderId"),
            json.GetInt("shortageQuantity"),
            json.GetInt("actualProductionQuantity"),
            Model::ProductionQueueItem::MinutesDuration(json.GetNumber("totalProductionMinutes")),
            FromEpochMilliseconds(json.GetInt64("productionStartEpochMillis")));
    }

    bool JsonProductionQueueRepository::Create(const Model::ProductionQueueItem& item)
    {
        if (FindById(item.GetOrderId()).has_value())
        {
            return false; // 동일 주문에 대한 생산 작업이 이미 존재함(병합 금지, PRD 4.4.2)
        }

        items_.push_back(item);
        Persist();
        return true;
    }

    std::vector<Model::ProductionQueueItem> JsonProductionQueueRepository::GetAll() const
    {
        return items_;
    }

    std::optional<Model::ProductionQueueItem> JsonProductionQueueRepository::FindById(const std::string& orderId) const
    {
        const auto found = std::find_if(items_.begin(), items_.end(),
            [&orderId](const Model::ProductionQueueItem& item) { return item.GetOrderId() == orderId; });

        if (found == items_.end())
        {
            return std::nullopt;
        }
        return *found;
    }

    bool JsonProductionQueueRepository::Update(const Model::ProductionQueueItem& item)
    {
        const auto found = std::find_if(items_.begin(), items_.end(),
            [&item](const Model::ProductionQueueItem& existing) { return existing.GetOrderId() == item.GetOrderId(); });

        if (found == items_.end())
        {
            return false;
        }

        *found = item;
        Persist();
        return true;
    }

    bool JsonProductionQueueRepository::Remove(const std::string& orderId)
    {
        const auto found = std::find_if(items_.begin(), items_.end(),
            [&orderId](const Model::ProductionQueueItem& item) { return item.GetOrderId() == orderId; });

        if (found == items_.end())
        {
            return false;
        }

        items_.erase(found);
        Persist();
        return true;
    }
}
