#pragma once

#include "../Controller/MainMenuController.h"
#include "MonitoringView.h"
#include "OrderApprovalView.h"
#include "OrderView.h"
#include "ProductionLineView.h"
#include "SampleView.h"
#include "ShipmentView.h"

namespace View
{
    // 최상위 메인 메뉴. 시료 관리/주문 접수/승인·거절/생산 라인/출고 처리/모니터링 하위 화면으로의
    // 라우팅을 담당한다(docs/PRD.md 4.1). Controller는 View를 알지 못하므로, 이미 조립된 하위 View들에
    // 대한 참조를 직접 받아 각 하위 메뉴 루프를 호출하는 방식으로 라우팅한다.
    class MainMenuView
    {
    public:
        MainMenuView(
            Controller::MainMenuController& mainMenuController,
            SampleView& sampleView,
            OrderView& orderView,
            OrderApprovalView& orderApprovalView,
            ProductionLineView& productionLineView,
            ShipmentView& shipmentView,
            MonitoringView& monitoringView);

        // 메인 메뉴 루프를 시작한다. 사용자가 "종료(0)"를 선택할 때까지 반복한다.
        void Run();

    private:
        void PrintSummary() const;

        Controller::MainMenuController& mainMenuController_;
        SampleView& sampleView_;
        OrderView& orderView_;
        OrderApprovalView& orderApprovalView_;
        ProductionLineView& productionLineView_;
        ShipmentView& shipmentView_;
        MonitoringView& monitoringView_;
    };
}
