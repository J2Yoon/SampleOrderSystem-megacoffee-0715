#pragma once

#include <string>
#include <vector>

#include "../Json/JsonValue.h"
#include "ISampleRepository.h"

namespace Persistence
{
    // ISampleRepository의 JSON 파일 기반 구현체.
    // 생성자에서 파일을 읽어(Load) 메모리에 적재하고, 변경(Create/Update/Remove)될 때마다
    // 즉시 파일 전체를 다시 기록한다(Persist, write-through). 파일이 없거나 파싱에 실패하면
    // 예외를 던지지 않고 빈 목록으로 시작한다.
    // 파일 스키마는 docs/PRD.md 5.4절의 PoC 호환 스키마(id/name/avgProductionMinutesPerUnit/yield/stock)를 따른다.
    class JsonSampleRepository : public ISampleRepository
    {
    public:
        explicit JsonSampleRepository(std::string filePath);

        bool Create(const Model::Sample& sample) override;
        std::vector<Model::Sample> GetAll() const override;
        std::optional<Model::Sample> FindById(const std::string& sampleId) const override;
        bool Update(const Model::Sample& sample) override;
        bool Remove(const std::string& sampleId) override;

    private:
        void Load();
        void Persist() const;

        static Json::Value ToJson(const Model::Sample& sample);
        static Model::Sample FromJson(const Json::Value& json);

        std::string filePath_;
        std::vector<Model::Sample> samples_;
    };
}
