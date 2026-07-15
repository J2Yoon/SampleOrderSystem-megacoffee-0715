#pragma once

#include <chrono>
#include <string>

namespace Model
{
    // 생산 큐 항목(ProductionQueueItem) — docs/PRD.md 5.3 참고
    // 순수 데이터 모델이며 콘솔 입출력, 파일 I/O에 의존하지 않는다.
    // 큐에 진입한 이후 취소/변경이 불가하다는 도메인 규칙(PRD 4.4.2)을 반영해 setter를 두지 않는다.
    class ProductionQueueItem
    {
    public:
        using TimePoint = std::chrono::system_clock::time_point;
        using MinutesDuration = std::chrono::duration<double, std::ratio<60>>;

        // productionStartTime + totalProductionMinutes로 예상 완료 시각을 계산해 생성한다.
        ProductionQueueItem(
            std::string orderId,
            int shortageQuantity,
            int actualProductionQuantity,
            MinutesDuration totalProductionTime,
            TimePoint productionStartTime);

        const std::string& GetOrderId() const;
        int GetShortageQuantity() const;
        int GetActualProductionQuantity() const;
        MinutesDuration GetTotalProductionTime() const;
        TimePoint GetProductionStartTime() const;
        TimePoint GetExpectedCompletionTime() const;

    private:
        std::string orderId_;
        int shortageQuantity_;
        int actualProductionQuantity_;
        MinutesDuration totalProductionTime_;
        TimePoint productionStartTime_;
        TimePoint expectedCompletionTime_;
    };
}
