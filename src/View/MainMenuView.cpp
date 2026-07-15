#include "MainMenuView.h"

#include <string>

#include "ConsoleView.h"

namespace View
{
    namespace
    {
        constexpr int kMenuChoiceSampleManagement = 1;
        constexpr int kMenuChoiceOrderManagement = 2;
        constexpr int kMenuChoiceOrderApproval = 3;
        constexpr int kMenuChoiceProductionLine = 4;
        constexpr int kMenuChoiceShipment = 5;
        constexpr int kMenuChoiceMonitoring = 6;
        constexpr int kMenuChoiceExit = 0;
    }

    MainMenuView::MainMenuView(
        Controller::MainMenuController& mainMenuController,
        SampleView& sampleView,
        OrderView& orderView,
        OrderApprovalView& orderApprovalView,
        ProductionLineView& productionLineView,
        ShipmentView& shipmentView,
        MonitoringView& monitoringView)
        : mainMenuController_(mainMenuController)
        , sampleView_(sampleView)
        , orderView_(orderView)
        , orderApprovalView_(orderApprovalView)
        , productionLineView_(productionLineView)
        , shipmentView_(shipmentView)
        , monitoringView_(monitoringView)
    {
    }

    void MainMenuView::PrintSummary() const
    {
        const auto summary = mainMenuController_.GetSummary();
        ConsoleView::PrintLine(
            "[요약] 등록 시료: " + std::to_string(summary.registeredSampleCount) + "종, 총 재고: "
            + std::to_string(summary.totalStock) + "개, 전체 주문: " + std::to_string(summary.totalOrderCount)
            + "건, 생산라인 대기: " + std::to_string(summary.productionQueuePendingItemCount) + "건");
    }

    void MainMenuView::Run()
    {
        ConsoleView::PrintTitle("반도체 시료 생산주문관리 시스템");

        bool isRunning = true;
        while (isRunning)
        {
            ConsoleView::PrintLine();
            PrintSummary();
            ConsoleView::PrintLine(
                "[1] 시료 관리   [2] 주문 접수   [3] 주문 승인/거절   [4] 생산 라인   "
                "[5] 출고 처리   [6] 모니터링   [0] 종료");
            const int choice = ConsoleView::ReadInt("선택 > ");

            switch (choice)
            {
            case kMenuChoiceSampleManagement:
                while (sampleView_.ShowMenuAndHandleSelection())
                {
                }
                break;
            case kMenuChoiceOrderManagement:
                while (orderView_.ShowMenuAndHandleSelection())
                {
                }
                break;
            case kMenuChoiceOrderApproval:
                while (orderApprovalView_.ShowMenuAndHandleSelection())
                {
                }
                break;
            case kMenuChoiceProductionLine:
                while (productionLineView_.ShowMenuAndHandleSelection())
                {
                }
                break;
            case kMenuChoiceShipment:
                while (shipmentView_.ShowMenuAndHandleSelection())
                {
                }
                break;
            case kMenuChoiceMonitoring:
                while (monitoringView_.ShowMenuAndHandleSelection())
                {
                }
                break;
            case kMenuChoiceExit:
                isRunning = false;
                break;
            default:
                ConsoleView::PrintError("올바른 번호를 선택해주세요.");
                break;
            }
        }
    }
}
