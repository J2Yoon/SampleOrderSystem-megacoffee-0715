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

#include "Controller/OrderApprovalController.h"
#include "Controller/OrderController.h"
#include "Controller/ProductionLineController.h"
#include "Controller/SampleController.h"
#include "Controller/ShipmentController.h"
#include "Persistence/JsonOrderRepository.h"
#include "Persistence/JsonProductionQueueRepository.h"
#include "Persistence/JsonSampleRepository.h"
#include "View/ConsoleView.h"
#include "View/OrderApprovalView.h"
#include "View/OrderView.h"
#include "View/ProductionLineView.h"
#include "View/SampleView.h"
#include "View/ShipmentView.h"

namespace
{
    constexpr int kTopMenuChoiceSampleManagement = 1;
    constexpr int kTopMenuChoiceOrderManagement = 2;
    constexpr int kTopMenuChoiceOrderApproval = 3;
    constexpr int kTopMenuChoiceProductionLine = 4;
    constexpr int kTopMenuChoiceShipment = 5;
    constexpr int kTopMenuChoiceExit = 0;
}

// Phase 9(메인 메뉴 통합)에서 MainMenuController/View로 대체될 임시 진입점이다.
// 현재는 Phase 6까지 구현된 시료 관리/주문 접수/주문 승인·거절/생산 라인 기능만 콘솔에서 수동으로
// 검증할 수 있게 배선한다.
//
// DI 조립 순서가 중요하다: OrderController/OrderApprovalController/ProductionLineController 모두
// SampleController에 의존하므로 SampleController가 먼저 생성되어야 한다(docs_temp/phase_4.md 설계 결정 1,
// docs_temp/phase_5.md 설계 결정 2 참고).
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

    // 앱 시작(재시작 포함) 시점의 현재 시각을 기준으로, 그동안 완료됐어야 할 생산 큐 항목을 즉시
    // 일괄 정산한다(docs/PRD.md 4.6.4 — 백그라운드 타이머 없이 재시작 시점 기준으로 정산).
    productionLineController.SettleCompletedItems();

    View::ConsoleView::PrintTitle("반도체 시료 생산주문관리 시스템 (Phase 7 - 시료/주문/승인·거절/생산 라인/출고)");
    View::ConsoleView::PrintLine("모니터링 메뉴는 이후 Phase에서 통합될 예정입니다.");

    bool isRunning = true;
    while (isRunning)
    {
        View::ConsoleView::PrintLine();
        View::ConsoleView::PrintLine(
            "[1] 시료 관리   [2] 주문 접수   [3] 주문 승인/거절   [4] 생산 라인   [5] 출고 처리   [0] 종료");
        const int choice = View::ConsoleView::ReadInt("선택 > ");

        switch (choice)
        {
        case kTopMenuChoiceSampleManagement:
            while (sampleView.ShowMenuAndHandleSelection())
            {
            }
            break;
        case kTopMenuChoiceOrderManagement:
            while (orderView.ShowMenuAndHandleSelection())
            {
            }
            break;
        case kTopMenuChoiceOrderApproval:
            while (orderApprovalView.ShowMenuAndHandleSelection())
            {
            }
            break;
        case kTopMenuChoiceProductionLine:
            while (productionLineView.ShowMenuAndHandleSelection())
            {
            }
            break;
        case kTopMenuChoiceShipment:
            while (shipmentView.ShowMenuAndHandleSelection())
            {
            }
            break;
        case kTopMenuChoiceExit:
            isRunning = false;
            break;
        default:
            View::ConsoleView::PrintError("올바른 번호를 선택해주세요.");
            break;
        }
    }

    return 0;
}

#endif
