#pragma once

#include "../Model/ProductionQueueItem.h"

namespace Controller
{
    // 생산량/생산시간 계산 공식(docs/PRD.md 4.6.1, 7절)을 한 곳에 모은 순수 계산 함수 모음.
    // 부작용이 없으며(콘솔/파일 I/O 없음) OrderApprovalController(승인 시점 큐 항목 생성)와
    // ProductionLineController(생산 라인) 양쪽에서 공유해 계산식이 중복 구현되지 않도록 한다(DRY).
    namespace ProductionCalculator
    {
        // 실 생산량 = ceil(부족분 / 수율)
        int CalculateActualProductionQuantity(int shortageQuantity, double yield);

        // 총 생산 시간 = 평균 생산시간 × 실 생산량
        Model::ProductionQueueItem::MinutesDuration CalculateTotalProductionTime(
            double averageProductionMinutesPerUnit, int actualProductionQuantity);
    }
}
