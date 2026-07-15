#pragma once

#include <string>

namespace Model
{
    // 시료(Sample) — docs/PRD.md 4.2.1 / 5.1 참고
    // 순수 데이터 모델이며 콘솔 입출력, 파일 I/O에 의존하지 않는다.
    class Sample
    {
    public:
        Sample(
            std::string sampleId,
            std::string name,
            double averageProductionMinutesPerUnit,
            double yield,
            int stock);

        const std::string& GetSampleId() const;
        const std::string& GetName() const;
        double GetAverageProductionMinutesPerUnit() const;
        double GetYield() const;
        int GetStock() const;

        // 재고는 생산 완료 시(+) 또는 출고 처리 시(-)에만 갱신되며, 실제 증감 로직은
        // Controller 계층(Phase 6/7)이 담당한다. 여기서는 결과 값을 반영하는 역할만 한다.
        void SetStock(int stock);

    private:
        std::string sampleId_;
        std::string name_;
        double averageProductionMinutesPerUnit_;
        double yield_;
        int stock_;
    };
}
