#pragma once

#include <string>

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

    // OrderStatus <-> 대문자 영문 문자열("RESERVED" 등, docs/PRD.md/CLAUDE.md 데이터 스키마 규약) 변환.
    // Persistence(JSON 직렬화)와 View(화면 표시)가 동일한 표준 표기를 공유하도록 단일 정의로 제공한다.
    // 알 수 없는 값이 들어오면 ToString은 예외를 던지고, FromString은 Reserved로 안전하게 폴백한다.
    std::string OrderStatusToString(OrderStatus status);
    OrderStatus OrderStatusFromString(const std::string& statusText);
}
