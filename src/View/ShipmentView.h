#pragma once

#include "../Controller/ShipmentController.h"

namespace View
{
    // 출고 처리 메뉴의 콘솔 입출력을 담당하는 View.
    // 도메인 로직을 갖지 않으며, 사용자 입력을 ShipmentController에 위임하고 그 결과만 표시한다.
    class ShipmentView
    {
    public:
        explicit ShipmentView(Controller::ShipmentController& shipmentController);

        // 출고 처리 메뉴를 한 번 표시하고 선택 항목을 처리한다.
        // 사용자가 "뒤로(0)"를 선택하면 false, 그 외에는 true를 반환한다.
        bool ShowMenuAndHandleSelection();

    private:
        void HandleListShippable() const;
        void HandleShip();

        Controller::ShipmentController& shipmentController_;
    };
}
