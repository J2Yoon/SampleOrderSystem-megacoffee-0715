#include "OrderApprovalView.h"

#include <iomanip>
#include <iostream>
#include <string>

#include "ConsoleView.h"

namespace View
{
    namespace
    {
        constexpr int kMenuChoiceListReserved = 1;
        constexpr int kMenuChoiceApprove = 2;
        constexpr int kMenuChoiceReject = 3;
        constexpr int kMenuChoiceBack = 0;
    }

    OrderApprovalView::OrderApprovalView(Controller::OrderApprovalController& orderApprovalController)
        : orderApprovalController_(orderApprovalController)
    {
    }

    bool OrderApprovalView::ShowMenuAndHandleSelection()
    {
        ConsoleView::PrintTitle("[3] 주문 승인/거절");
        ConsoleView::PrintLine("[1] 접수 대기 목록   [2] 승인   [3] 거절   [0] 뒤로");
        const int choice = ConsoleView::ReadInt("선택 > ");

        switch (choice)
        {
        case kMenuChoiceListReserved:
            HandleListReserved();
            return true;
        case kMenuChoiceApprove:
            HandleApprove();
            return true;
        case kMenuChoiceReject:
            HandleReject();
            return true;
        case kMenuChoiceBack:
            return false;
        default:
            ConsoleView::PrintError("올바른 번호를 선택해주세요.");
            return true;
        }
    }

    void OrderApprovalView::HandleListReserved() const
    {
        const auto reservedOrders = orderApprovalController_.GetReservedOrders();
        ConsoleView::PrintLine("접수 대기(RESERVED) 목록 (총 " + std::to_string(reservedOrders.size()) + "건)");
        PrintOrderTableHeader();
        for (const auto& order : reservedOrders)
        {
            PrintOrderRow(order);
        }
    }

    void OrderApprovalView::HandleApprove()
    {
        const std::string orderId = ConsoleView::ReadLine("승인할 주문번호 > ");
        const auto result = orderApprovalController_.ApproveOrder(orderId);

        switch (result)
        {
        case Controller::OrderApprovalResult::Success:
            ConsoleView::PrintLine("승인 처리 완료(재고 상황에 따라 CONFIRMED 또는 PRODUCING으로 전환).");
            break;
        case Controller::OrderApprovalResult::OrderNotFound:
            ConsoleView::PrintError("존재하지 않는 주문번호입니다.");
            break;
        case Controller::OrderApprovalResult::InvalidOrderState:
            ConsoleView::PrintError("RESERVED 상태의 주문만 승인할 수 있습니다.");
            break;
        case Controller::OrderApprovalResult::SampleNotFound:
            ConsoleView::PrintError("주문이 참조하는 시료를 찾을 수 없습니다.");
            break;
        }
    }

    void OrderApprovalView::HandleReject()
    {
        const std::string orderId = ConsoleView::ReadLine("거절할 주문번호 > ");
        const auto result = orderApprovalController_.RejectOrder(orderId);

        switch (result)
        {
        case Controller::OrderRejectionResult::Success:
            ConsoleView::PrintLine("거절 처리 완료(REJECTED).");
            break;
        case Controller::OrderRejectionResult::OrderNotFound:
            ConsoleView::PrintError("존재하지 않는 주문번호입니다.");
            break;
        case Controller::OrderRejectionResult::InvalidOrderState:
            ConsoleView::PrintError("이미 최종 상태이거나 거절할 수 없는 주문입니다.");
            break;
        }
    }

    void OrderApprovalView::PrintOrderTableHeader()
    {
        std::cout << std::left << std::setw(20) << "주문번호" << std::setw(10) << "시료ID"
            << std::setw(16) << "고객명" << std::setw(8) << "수량" << "상태\n";
    }

    void OrderApprovalView::PrintOrderRow(const Model::Order& order)
    {
        std::cout << std::left << std::setw(20) << order.GetOrderId() << std::setw(10) << order.GetSampleId()
            << std::setw(16) << order.GetCustomerName() << std::setw(8) << order.GetQuantity() << "RESERVED\n";
    }
}
