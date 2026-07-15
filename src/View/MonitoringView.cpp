#include "MonitoringView.h"

#include <array>
#include <string>

#include "ConsoleView.h"

namespace View
{
    namespace
    {
        constexpr int kMenuChoiceOrderCounts = 1;
        constexpr int kMenuChoiceStockStatuses = 2;
        constexpr int kMenuChoiceBack = 0;

        // docs/PRD.md 4.5.1에 명시된 표시 순서(RESERVED/CONFIRMED/PRODUCING/RELEASED)를 그대로 따른다.
        constexpr std::array<Model::OrderStatus, 4> kOrderCountDisplayOrder
        {
            Model::OrderStatus::Reserved,
            Model::OrderStatus::Confirmed,
            Model::OrderStatus::Producing,
            Model::OrderStatus::Released
        };

        std::string StockLevelToDisplayText(Controller::StockLevel level)
        {
            switch (level)
            {
            case Controller::StockLevel::Sufficient:
                return "여유";
            case Controller::StockLevel::Low:
                return "부족";
            case Controller::StockLevel::Depleted:
                return "고갈";
            }
            return "알 수 없음";
        }
    }

    MonitoringView::MonitoringView(Controller::MonitoringController& monitoringController)
        : monitoringController_(monitoringController)
    {
    }

    bool MonitoringView::ShowMenuAndHandleSelection()
    {
        ConsoleView::PrintTitle("[6] 모니터링");
        ConsoleView::PrintLine("[1] 상태별 주문 현황   [2] 시료별 재고 현황   [0] 뒤로");
        const int choice = ConsoleView::ReadInt("선택 > ");

        switch (choice)
        {
        case kMenuChoiceOrderCounts:
            HandleShowOrderCounts();
            return true;
        case kMenuChoiceStockStatuses:
            HandleShowStockStatuses();
            return true;
        case kMenuChoiceBack:
            return false;
        default:
            ConsoleView::PrintError("올바른 번호를 선택해주세요.");
            return true;
        }
    }

    void MonitoringView::HandleShowOrderCounts() const
    {
        const auto orderCountsByStatus = monitoringController_.GetOrderCountsByStatus();
        ConsoleView::PrintLine("상태별 주문 현황 (REJECTED 제외):");
        for (const auto status : kOrderCountDisplayOrder)
        {
            const auto foundStatusCount = orderCountsByStatus.find(status);
            const int count = (foundStatusCount != orderCountsByStatus.end()) ? foundStatusCount->second : 0;
            ConsoleView::PrintLine("  " + Model::OrderStatusToString(status) + ": " + std::to_string(count) + "건");
        }
    }

    void MonitoringView::HandleShowStockStatuses() const
    {
        const auto stockStatuses = monitoringController_.GetStockStatuses();
        ConsoleView::PrintLine("시료별 재고 현황:");
        for (const auto& stockStatus : stockStatuses)
        {
            ConsoleView::PrintLine(
                "  " + stockStatus.sample.GetSampleId() + " (" + stockStatus.sample.GetName() + ") - 재고: "
                + std::to_string(stockStatus.sample.GetStock()) + ", 미출고 주문 수량: "
                + std::to_string(stockStatus.pendingDemand) + ", 상태: " + StockLevelToDisplayText(stockStatus.level));
        }
    }
}
