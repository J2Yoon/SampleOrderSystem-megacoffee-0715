#include "OrderView.h"

#include <iomanip>
#include <iostream>
#include <string>

#include "ConsoleView.h"

namespace View
{
    namespace
    {
        constexpr int kMenuChoicePlaceOrder = 1;
        constexpr int kMenuChoiceListAll = 2;
        constexpr int kMenuChoiceBack = 0;
    }

    OrderView::OrderView(Controller::OrderController& orderController)
        : orderController_(orderController)
    {
    }

    bool OrderView::ShowMenuAndHandleSelection()
    {
        ConsoleView::PrintTitle("[2] 주문 접수");
        ConsoleView::PrintLine("[1] 주문 접수   [2] 전체 주문 목록   [0] 뒤로");
        const int choice = ConsoleView::ReadInt("선택 > ");

        switch (choice)
        {
        case kMenuChoicePlaceOrder:
            HandlePlaceOrder();
            return true;
        case kMenuChoiceListAll:
            HandleListAll();
            return true;
        case kMenuChoiceBack:
            return false;
        default:
            ConsoleView::PrintError("올바른 번호를 선택해주세요.");
            return true;
        }
    }

    void OrderView::HandlePlaceOrder()
    {
        const std::string sampleId = ConsoleView::ReadLine("시료 ID   > ");
        const std::string customerName = ConsoleView::ReadLine("고객명    > ");
        const int quantity = ConsoleView::ReadInt("주문 수량 > ");

        const auto result = orderController_.PlaceOrder(sampleId, customerName, quantity);

        switch (result)
        {
        case Controller::OrderPlacementResult::Success:
            ConsoleView::PrintLine("주문 접수 완료(RESERVED).");
            break;
        case Controller::OrderPlacementResult::UnregisteredSample:
            ConsoleView::PrintError("등록되지 않은 시료입니다. 먼저 시료 관리에서 시료를 등록해주세요.");
            break;
        case Controller::OrderPlacementResult::InvalidInput:
            ConsoleView::PrintError("입력값을 확인해주세요(고객명은 비어있을 수 없고, 주문 수량은 1 이상이어야 합니다).");
            break;
        }
    }

    void OrderView::HandleListAll() const
    {
        const auto orders = orderController_.GetAllOrders();
        ConsoleView::PrintLine("전체 주문 목록 (총 " + std::to_string(orders.size()) + "건)");
        PrintOrderTableHeader();
        for (const auto& order : orders)
        {
            PrintOrderRow(order);
        }
    }

    void OrderView::PrintOrderTableHeader()
    {
        std::cout << std::left << std::setw(20) << "주문번호" << std::setw(10) << "시료ID"
            << std::setw(16) << "고객명" << std::setw(8) << "수량" << "상태\n";
    }

    void OrderView::PrintOrderRow(const Model::Order& order)
    {
        std::cout << std::left << std::setw(20) << order.GetOrderId() << std::setw(10) << order.GetSampleId()
            << std::setw(16) << order.GetCustomerName() << std::setw(8) << order.GetQuantity()
            << OrderStatusToDisplayText(order.GetStatus()) << "\n";
    }

    const char* OrderView::OrderStatusToDisplayText(Model::OrderStatus status)
    {
        switch (status)
        {
        case Model::OrderStatus::Reserved:
            return "RESERVED";
        case Model::OrderStatus::Rejected:
            return "REJECTED";
        case Model::OrderStatus::Producing:
            return "PRODUCING";
        case Model::OrderStatus::Confirmed:
            return "CONFIRMED";
        case Model::OrderStatus::Released:
            return "RELEASED";
        }
        return "UNKNOWN";
    }
}
