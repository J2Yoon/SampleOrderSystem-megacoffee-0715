#include <windows.h>

#ifdef _DEBUG

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#else

#include "Controller/MainMenuController.h"
#include "Controller/MonitoringController.h"
#include "Controller/OrderApprovalController.h"
#include "Controller/OrderController.h"
#include "Controller/ProductionLineController.h"
#include "Controller/SampleController.h"
#include "Controller/ShipmentController.h"
#include "Persistence/JsonOrderRepository.h"
#include "Persistence/JsonProductionQueueRepository.h"
#include "Persistence/JsonSampleRepository.h"
#include "View/MainMenuView.h"
#include "View/MonitoringView.h"
#include "View/OrderApprovalView.h"
#include "View/OrderView.h"
#include "View/ProductionLineView.h"
#include "View/SampleView.h"
#include "View/ShipmentView.h"

// 애플리케이션 진입점. 시료 관리/주문 접수/주문 승인·거절/생산 라인/출고 처리/모니터링 기능을
// MainMenuView로 통합해 콘솔에서 하나의 메인 메뉴로 이용할 수 있게 배선한다(docs/PRD.md 4.1).
//
// DI 조립 순서가 중요하다: OrderController/OrderApprovalController/ProductionLineController/
// MainMenuController 모두 SampleController에 의존하므로 SampleController가 먼저 생성되어야 한다
// (docs_temp/phase_4.md 설계 결정 1, docs_temp/phase_5.md 설계 결정 2 참고).
int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    Persistence::JsonSampleRepository sampleRepository("data/samples.json");
    Controller::SampleController sampleController(sampleRepository);
    View::SampleView sampleView(sampleController);

    Persistence::JsonOrderRepository orderRepository("data/orders.json");
    Controller::OrderController orderController(orderRepository, sampleController);
    View::OrderView orderView(orderController);

    Persistence::JsonProductionQueueRepository productionQueueRepository("data/productionQueue.json");
    Controller::OrderApprovalController orderApprovalController(
        orderRepository, sampleController, productionQueueRepository);
    View::OrderApprovalView orderApprovalView(orderApprovalController);

    Controller::ProductionLineController productionLineController(
        productionQueueRepository, orderRepository, sampleController);
    View::ProductionLineView productionLineView(productionLineController);

    Controller::ShipmentController shipmentController(orderRepository, sampleController);
    View::ShipmentView shipmentView(shipmentController);

    Controller::MonitoringController monitoringController(orderRepository, sampleController);
    View::MonitoringView monitoringView(monitoringController);

    Controller::MainMenuController mainMenuController(sampleController, orderRepository, productionLineController);

    // 앱 시작(재시작 포함) 시점의 현재 시각을 기준으로, 그동안 완료됐어야 할 생산 큐 항목을 즉시
    // 일괄 정산한다(docs/PRD.md 4.6.4 — 백그라운드 타이머 없이 재시작 시점 기준으로 정산).
    // MainMenuController의 요약 정보(생산라인 대기 건수)가 이 정산 이후 상태를 반영하도록 반드시 먼저 호출한다.
    productionLineController.SettleCompletedItems();

    View::MainMenuView mainMenuView(
        mainMenuController,
        sampleView,
        orderView,
        orderApprovalView,
        productionLineView,
        shipmentView,
        monitoringView);
    mainMenuView.Run();

    return 0;
}

#endif
