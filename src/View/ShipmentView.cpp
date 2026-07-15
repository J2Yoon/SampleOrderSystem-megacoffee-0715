#include "ShipmentView.h"

#include <string>

#include "ConsoleView.h"

namespace View
{
    namespace
    {
        constexpr int kMenuChoiceListShippable = 1;
        constexpr int kMenuChoiceShip = 2;
        constexpr int kMenuChoiceBack = 0;
    }

    ShipmentView::ShipmentView(Controller::ShipmentController& shipmentController)
        : shipmentController_(shipmentController)
    {
    }

    bool ShipmentView::ShowMenuAndHandleSelection()
    {
        ConsoleView::PrintTitle("[5] 출고 처리");
        ConsoleView::PrintLine("[1] 출고 가능 목록   [2] 출고 처리   [0] 뒤로");
        const int choice = ConsoleView::ReadInt("선택 > ");

        switch (choice)
        {
        case kMenuChoiceListShippable:
            HandleListShippable();
            return true;
        case kMenuChoiceShip:
            HandleShip();
            return true;
        case kMenuChoiceBack:
            return false;
        default:
            ConsoleView::PrintError("올바른 번호를 선택해주세요.");
            return true;
        }
    }

    void ShipmentView::HandleListShippable() const
    {
        const auto shippableOrders = shipmentController_.GetShippableOrders();
        ConsoleView::PrintLine("출고 가능(CONFIRMED) 목록 (총 " + std::to_string(shippableOrders.size()) + "건)");
        ConsoleView::PrintOrderTableHeader();
        for (const auto& order : shippableOrders)
        {
            ConsoleView::PrintOrderRow(order);
        }
    }

    void ShipmentView::HandleShip()
    {
        const std::string orderId = ConsoleView::ReadLine("출고 처리할 주문번호 > ");
        const auto result = shipmentController_.ShipOrder(orderId);

        switch (result)
        {
        case Controller::ShipmentResult::Success:
            ConsoleView::PrintLine("출고 처리 완료(CONFIRMED -> RELEASED, 재고 차감).");
            break;
        case Controller::ShipmentResult::OrderNotFound:
            ConsoleView::PrintError("존재하지 않는 주문번호입니다.");
            break;
        case Controller::ShipmentResult::InvalidOrderState:
            ConsoleView::PrintError("CONFIRMED 상태의 주문만 출고할 수 있습니다.");
            break;
        case Controller::ShipmentResult::SampleNotFound:
            ConsoleView::PrintError("주문이 참조하는 시료를 찾을 수 없습니다.");
            break;
        }
    }
}
