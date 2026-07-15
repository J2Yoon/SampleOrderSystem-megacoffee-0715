#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../Model/Sample.h"

namespace Persistence
{
    // 시료(Sample) 데이터에 대한 CRUD 인터페이스. docs/PRD.md 5.1 / 5.4 참고.
    class ISampleRepository
    {
    public:
        virtual ~ISampleRepository() = default;

        // 이미 등록된 시료 ID면 false를 반환하고 아무 것도 하지 않는다.
        virtual bool Create(const Model::Sample& sample) = 0;
        virtual std::vector<Model::Sample> GetAll() const = 0;
        virtual std::optional<Model::Sample> FindById(const std::string& sampleId) const = 0;
        // 대상 시료 ID가 없으면 false를 반환한다.
        virtual bool Update(const Model::Sample& sample) = 0;
        // 대상 시료 ID가 없으면 false를 반환한다.
        virtual bool Remove(const std::string& sampleId) = 0;
    };
}
