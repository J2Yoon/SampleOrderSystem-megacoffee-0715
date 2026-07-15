#include "JsonSampleRepository.h"

#include <algorithm>

#include "../Json/JsonIO.h"

namespace Persistence
{
    JsonSampleRepository::JsonSampleRepository(std::string filePath)
        : filePath_(std::move(filePath))
    {
        Load();
    }

    void JsonSampleRepository::Load()
    {
        samples_.clear();

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
            samples_.push_back(FromJson(item));
        }
    }

    void JsonSampleRepository::Persist() const
    {
        Json::Value root = Json::Value::MakeArray();
        for (const auto& sample : samples_)
        {
            root.Add(ToJson(sample));
        }
        Json::FileIO::WriteAllText(filePath_, root.Dump());
    }

    Json::Value JsonSampleRepository::ToJson(const Model::Sample& sample)
    {
        Json::Value json = Json::Value::MakeObject();
        json.Set("id", Json::Value::MakeString(sample.GetSampleId()));
        json.Set("name", Json::Value::MakeString(sample.GetName()));
        json.Set("avgProductionMinutesPerUnit", Json::Value::MakeNumber(sample.GetAverageProductionMinutesPerUnit()));
        json.Set("yield", Json::Value::MakeNumber(sample.GetYield()));
        json.Set("stock", Json::Value::MakeNumber(sample.GetStock()));
        return json;
    }

    Model::Sample JsonSampleRepository::FromJson(const Json::Value& json)
    {
        return Model::Sample(
            json.GetString("id"),
            json.GetString("name"),
            json.GetNumber("avgProductionMinutesPerUnit"),
            json.GetNumber("yield"),
            json.GetInt("stock"));
    }

    bool JsonSampleRepository::Create(const Model::Sample& sample)
    {
        if (FindById(sample.GetSampleId()).has_value())
        {
            return false; // 이미 등록된 시료 ID
        }

        samples_.push_back(sample);
        Persist();
        return true;
    }

    std::vector<Model::Sample> JsonSampleRepository::GetAll() const
    {
        return samples_;
    }

    std::optional<Model::Sample> JsonSampleRepository::FindById(const std::string& sampleId) const
    {
        const auto found = std::find_if(samples_.begin(), samples_.end(),
            [&sampleId](const Model::Sample& sample) { return sample.GetSampleId() == sampleId; });

        if (found == samples_.end())
        {
            return std::nullopt;
        }
        return *found;
    }

    bool JsonSampleRepository::Update(const Model::Sample& sample)
    {
        const auto found = std::find_if(samples_.begin(), samples_.end(),
            [&sample](const Model::Sample& existing) { return existing.GetSampleId() == sample.GetSampleId(); });

        if (found == samples_.end())
        {
            return false;
        }

        *found = sample;
        Persist();
        return true;
    }

    bool JsonSampleRepository::Remove(const std::string& sampleId)
    {
        const auto found = std::find_if(samples_.begin(), samples_.end(),
            [&sampleId](const Model::Sample& sample) { return sample.GetSampleId() == sampleId; });

        if (found == samples_.end())
        {
            return false;
        }

        samples_.erase(found);
        Persist();
        return true;
    }
}
