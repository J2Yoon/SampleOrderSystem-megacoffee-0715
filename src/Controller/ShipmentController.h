#pragma once

#include <string>
#include <vector>

#include "../Model/Order.h"
#include "../Persistence/IOrderRepository.h"
#include "SampleController.h"

namespace Controller
{
    // 출고 처리 결과. 실패 원인을 구분해 View가 적절한 안내 메시지를 표시할 수 있게 한다.
    enum class ShipmentResult
    {
        Success,
        OrderNotFound,      // 존재하지 않는 주문번호
        InvalidOrderState,  // CONFIRMED 상태가 아니어서 출고할 수 없음
        SampleNotFound      // 주문이 참조하는 시료를 더 이상 찾을 수 없음(방어적 처리)
    };

    // 출고 처리 기능을 담당하는 Controller. docs/PRD.md 4.7 참고.
    // 콘솔 입출력(<iostream>)에 의존하지 않는다. 재고 차감은 SampleController에 위임하며,
    // 부분 출고는 지원하지 않는다(항상 주문 수량 전체를 출고).
    class ShipmentController
    {
    public:
        ShipmentController(
            Persistence::IOrderRepository& orderRepository,
            SampleController& sampleController);

        // CONFIRMED 상태의 주문만 필터링해 반환한다(docs/PRD.md 4.7 — 출고 가능 목록은 CONFIRMED로 제한).
        std::vector<Model::Order> GetShippableOrders() const;

        // 주문 전체 수량을 출고 처리한다. 재고를 출고 수량만큼 감소시키고 주문을 RELEASED로 전환한다.
        ShipmentResult ShipOrder(const std::string& orderId);

    private:
        Persistence::IOrderRepository& orderRepository_;
        SampleController& sampleController_;
    };
}
