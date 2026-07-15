#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../Model/Sample.h"
#include "../Persistence/ISampleRepository.h"

namespace Controller
{
    // 시료 등록 결과. 실패 원인을 구분해 View가 적절한 안내 메시지를 표시할 수 있게 한다.
    enum class SampleRegistrationResult
    {
        Success,
        InvalidInput,       // 시료 ID/이름이 비어있거나 평균 생산시간/수율이 유효 범위를 벗어남
        DuplicateSampleId   // 이미 등록된 시료 ID
    };

    // 시료 관리(등록/조회/검색) 기능을 담당하는 Controller. docs/PRD.md 4.2 참고.
    // 콘솔 입출력(<iostream>)에 의존하지 않으며, Persistence 계층을 통해서만 데이터에 접근한다.
    class SampleController
    {
    public:
        explicit SampleController(Persistence::ISampleRepository& sampleRepository);

        // 새 시료를 등록한다. 신규 시료의 초기 재고는 항상 0이다.
        SampleRegistrationResult RegisterSample(
            const std::string& sampleId,
            const std::string& name,
            double averageProductionMinutesPerUnit,
            double yield);

        // 등록된 모든 시료 목록(현재 재고 포함)을 조회한다.
        std::vector<Model::Sample> GetAllSamples() const;

        // 이름에 keyword가 포함된(대소문자 무시) 시료를 검색한다.
        std::vector<Model::Sample> SearchByName(const std::string& keyword) const;

        // 이후 단계(주문 접수 등)에서 미등록 시료를 참조하지 못하도록 검증할 때 사용한다(docs/PRD.md 4.2).
        bool IsSampleRegistered(const std::string& sampleId) const;

        // 주문 승인 시점의 재고 조회 등, 시료 데이터(재고 포함)를 그대로 참조해야 하는 경우에 사용한다.
        // 재고를 변경하지 않고 조회만 한다(docs/PRD.md 4.6.1).
        std::optional<Model::Sample> FindSampleById(const std::string& sampleId) const;

        // 생산 완료 시점에 실 생산량만큼 재고를 실제로 증가시킨다(docs/PRD.md 4.6.1).
        // 대상 시료가 없으면 false를 반환하고 아무 것도 하지 않는다.
        bool IncreaseStock(const std::string& sampleId, int increaseAmount);

        // 출고 처리 시점에 출고 수량만큼 재고를 실제로 감소시킨다(docs/PRD.md 4.6.1, 4.7).
        // 대상 시료가 없으면 false를 반환하고 아무 것도 하지 않는다.
        bool DecreaseStock(const std::string& sampleId, int decreaseAmount);

    private:
        static bool IsValidRegistrationInput(
            const std::string& sampleId,
            const std::string& name,
            double averageProductionMinutesPerUnit,
            double yield);

        Persistence::ISampleRepository& sampleRepository_;
    };
}
