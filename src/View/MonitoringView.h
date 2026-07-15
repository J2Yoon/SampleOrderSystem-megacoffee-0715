#pragma once

#include "../Controller/MonitoringController.h"

namespace View
{
    // 모니터링 메뉴(상태별 주문 현황 / 시료별 재고 현황)의 콘솔 입출력을 담당하는 View.
    // 도메인 로직을 갖지 않으며, MonitoringController가 계산한 결과를 표시만 한다.
    class MonitoringView
    {
    public:
        explicit MonitoringView(Controller::MonitoringController& monitoringController);

        // 모니터링 메뉴를 한 번 표시하고 선택 항목을 처리한다.
        // 사용자가 "뒤로(0)"를 선택하면 false, 그 외에는 true를 반환한다.
        bool ShowMenuAndHandleSelection();

    private:
        void HandleShowOrderCounts() const;
        void HandleShowStockStatuses() const;

        Controller::MonitoringController& monitoringController_;
    };
}
