#include "ProductionLineView.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

#include "ConsoleView.h"

namespace View
{
    namespace
    {
        constexpr int kMenuChoiceCurrentStatus = 1;
        constexpr int kMenuChoiceWaitingQueue = 2;
        constexpr int kMenuChoiceBack = 0;

        double ToElapsedMinutes(Model::ProductionQueueItem::TimePoint from, Model::ProductionQueueItem::TimePoint to)
        {
            return std::chrono::duration<double, std::ratio<60>>(to - from).count();
        }
    }

    ProductionLineView::ProductionLineView(Controller::ProductionLineController& productionLineController)
        : productionLineController_(productionLineController)
    {
    }

    bool ProductionLineView::ShowMenuAndHandleSelection()
    {
        productionLineController_.SettleCompletedItems();

        ConsoleView::PrintTitle("[4] 생산 라인");
        ConsoleView::PrintLine("[1] 현재 생산 현황   [2] 대기 큐 목록   [0] 뒤로");
        const int choice = ConsoleView::ReadInt("선택 > ");

        switch (choice)
        {
        case kMenuChoiceCurrentStatus:
            HandleShowCurrentStatus();
            return true;
        case kMenuChoiceWaitingQueue:
            HandleShowWaitingQueue();
            return true;
        case kMenuChoiceBack:
            return false;
        default:
            ConsoleView::PrintError("올바른 번호를 선택해주세요.");
            return true;
        }
    }

    void ProductionLineView::HandleShowCurrentStatus() const
    {
        const auto currentStatus = productionLineController_.GetCurrentProductionStatus();
        if (!currentStatus.has_value())
        {
            ConsoleView::PrintLine("현재 생산 중인 항목이 없습니다.");
            return;
        }

        const auto now = std::chrono::system_clock::now();
        const double elapsedMinutes = ToElapsedMinutes(currentStatus->effectiveStartTime, now);
        const double totalMinutes = currentStatus->item.GetTotalProductionTime().count();

        ConsoleView::PrintLine("주문번호: " + currentStatus->item.GetOrderId());
        ConsoleView::PrintLine("실 생산량: " + std::to_string(currentStatus->item.GetActualProductionQuantity()));
        std::cout << std::fixed << std::setprecision(1)
            << "생산 경과/총 소요 시간(분): " << elapsedMinutes << " / " << totalMinutes << "\n";
    }

    void ProductionLineView::HandleShowWaitingQueue() const
    {
        const auto waitingQueue = productionLineController_.GetWaitingQueue();
        ConsoleView::PrintLine("대기 큐 목록 (총 " + std::to_string(waitingQueue.size()) + "건, FIFO 순서)");
        for (const auto& item : waitingQueue)
        {
            PrintProductionQueueItemRow(item);
        }
    }

    void ProductionLineView::PrintProductionQueueItemRow(const Model::ProductionQueueItem& item)
    {
        std::cout << std::left << std::setw(20) << item.GetOrderId()
            << "실 생산량 " << std::setw(6) << item.GetActualProductionQuantity()
            << "총 소요(분) " << std::fixed << std::setprecision(1) << item.GetTotalProductionTime().count()
            << "\n";
    }
}
