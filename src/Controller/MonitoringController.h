#pragma once

#include <map>
#include <vector>

#include "../Model/OrderStatus.h"
#include "../Persistence/IOrderRepository.h"
#include "MonitoringService.h"
#include "SampleController.h"

namespace Controller
{
    // 모니터링(상태별 주문 현황 / 시료별 재고 현황) 기능을 담당하는 Controller. docs/PRD.md 4.5 참고.
    // 콘솔 입출력(<iostream>)에 의존하지 않으며, 실제 집계 계산은 MonitoringService에 위임한다.
    class MonitoringController
    {
    public:
        MonitoringController(
            Persistence::IOrderRepository& orderRepository,
            SampleController& sampleController);

        // 상태별(RESERVED/PRODUCING/CONFIRMED/RELEASED) 주문 건수를 반환한다. REJECTED는 제외된다.
        std::map<Model::OrderStatus, int> GetOrderCountsByStatus() const;

        // 시료별 재고 현황(여유/부족/고갈 포함)을 반환한다.
        std::vector<StockStatus> GetStockStatuses() const;

    private:
        Persistence::IOrderRepository& orderRepository_;
        SampleController& sampleController_;
    };
}
