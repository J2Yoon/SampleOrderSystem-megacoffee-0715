#include "JsonSampleRepository.h"

#include "JsonRepositoryUtil.h"

namespace Persistence
{
    namespace
    {
        std::string GetSampleKey(const Model::Sample& sample)
        {
            return sample.GetSampleId();
        }
    }

    JsonSampleRepository::JsonSampleRepository(std::string filePath)
        : filePath_(std::move(filePath))
    {
        Load();
    }

    void JsonSampleRepository::Load()
    {
        samples_ = JsonRepositoryUtil::LoadEntitiesFromFile<Model::Sample>(filePath_, &FromJson);
    }

    void JsonSampleRepository::Persist() const
    {
        JsonRepositoryUtil::PersistEntitiesToFile(filePath_, samples_, &ToJson);
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
        const auto found = JsonRepositoryUtil::FindIteratorByKey(samples_, sampleId, &GetSampleKey);

        if (found == samples_.end())
        {
            return std::nullopt;
        }
        return *found;
    }

    bool JsonSampleRepository::Update(const Model::Sample& sample)
    {
        const auto found = JsonRepositoryUtil::FindIteratorByKey(samples_, GetSampleKey(sample), &GetSampleKey);

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
        const auto found = JsonRepositoryUtil::FindIteratorByKey(samples_, sampleId, &GetSampleKey);

        if (found == samples_.end())
        {
            return false;
        }

        samples_.erase(found);
        Persist();
        return true;
    }
}
