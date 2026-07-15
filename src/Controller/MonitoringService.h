#pragma once

#include <map>
#include <vector>

#include "../Model/Order.h"
#include "../Model/OrderStatus.h"
#include "../Model/Sample.h"

namespace Controller
{
    // 재고 상태 : 해당 시료의 미출고 주문 수량(pendingDemand) 대비 재고 수량에 따라 표기되는 상태(docs/PRD.md 4.5.2)
    enum class StockLevel
    {
        Sufficient, // 여유
        Low,        // 부족
        Depleted    // 고갈
    };

    struct StockStatus
    {
        Model::Sample sample;
        int pendingDemand = 0; // 해당 시료에 대한 미출고 주문(RESERVED/PRODUCING/CONFIRMED) 수량 합
        StockLevel level = StockLevel::Sufficient;
    };

    // 이미 로드된 시료/주문 데이터를 바탕으로 모니터링용 집계 정보를 계산하는 순수 계산 함수 모음.
    // ProductionCalculator(Phase 6)와 동일하게 부작용이 없으며(콘솔/파일 I/O 없음), MonitoringController가
    // 이 함수들을 호출해 결과를 그대로 전달한다(docs/PRD.md 4.5).
    namespace MonitoringService
    {
        // 상태별 주문 건수를 집계한다. RESERVED/PRODUCING/CONFIRMED/RELEASED 4개 키만 포함하며,
        // REJECTED는 정상 흐름 외 상태이므로 집계에서 제외한다(docs/PRD.md 4.5.1).
        std::map<Model::OrderStatus, int> CountOrdersByStatus(const std::vector<Model::Order>& orders);

        // 시료별 재고 현황(미출고 주문 수량 대비 여유/부족/고갈)을 계산한다(docs/PRD.md 4.5.2).
        std::vector<StockStatus> BuildStockStatuses(
            const std::vector<Model::Sample>& samples, const std::vector<Model::Order>& orders);
    }
}
