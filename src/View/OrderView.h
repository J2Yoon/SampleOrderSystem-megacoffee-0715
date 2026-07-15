#pragma once

#include "../Controller/OrderController.h"

namespace View
{
    // 주문 접수 메뉴의 콘솔 입출력을 담당하는 View.
    // 도메인 로직을 갖지 않으며, 사용자 입력을 OrderController에 위임하고 그 결과만 표시한다.
    class OrderView
    {
    public:
        explicit OrderView(Controller::OrderController& orderController);

        // 주문 관리 메뉴를 한 번 표시하고 선택 항목을 처리한다.
        // 사용자가 "뒤로(0)"를 선택하면 false, 그 외에는 true를 반환한다.
        bool ShowMenuAndHandleSelection();

    private:
        void HandlePlaceOrder();
        void HandleListAll() const;

        static void PrintOrderTableHeader();
        static void PrintOrderRow(const Model::Order& order);
        static const char* OrderStatusToDisplayText(Model::OrderStatus status);

        Controller::OrderController& orderController_;
    };
}
