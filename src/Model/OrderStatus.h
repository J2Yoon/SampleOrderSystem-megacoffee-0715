#pragma once

namespace Model
{
    // 주문 상태 (docs/PRD.md 3절 참고)
    enum class OrderStatus
    {
        Reserved,   // 주문 접수
        Rejected,   // 주문 거절 (종단 상태, 모니터링 제외)
        Producing,  // 승인 완료 + 재고 부족으로 생산 중
        Confirmed,  // 승인 완료 + 출고 대기 중
        Released    // 출고 완료 (종단 상태)
    };

    // from -> to 로의 상태 전이가 허용되는지 검사한다.
    // 허용된 전이: RESERVED -> REJECTED, RESERVED -> CONFIRMED, RESERVED -> PRODUCING,
    //             PRODUCING -> CONFIRMED, CONFIRMED -> RELEASED
    // 그 외(REJECTED/RELEASED에서의 모든 전이, 자기 자신으로의 전이 등)는 모두 차단한다.
    bool IsValidOrderStatusTransition(OrderStatus from, OrderStatus to);
}
