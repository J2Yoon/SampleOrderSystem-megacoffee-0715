#include "OrderStatus.h"

#include <array>
#include <stdexcept>
#include <utility>

namespace Model
{
    namespace
    {
        using TransitionPair = std::pair<OrderStatus, OrderStatus>;

        // 허용된 상태 전이 테이블 (docs/PRD.md 3절 상태 전이 다이어그램 그대로 반영)
        constexpr std::array<TransitionPair, 5> kAllowedTransitions
        {
            TransitionPair{ OrderStatus::Reserved, OrderStatus::Rejected },
            TransitionPair{ OrderStatus::Reserved, OrderStatus::Confirmed },
            TransitionPair{ OrderStatus::Reserved, OrderStatus::Producing },
            TransitionPair{ OrderStatus::Producing, OrderStatus::Confirmed },
            TransitionPair{ OrderStatus::Confirmed, OrderStatus::Released },
        };

        using OrderStatusNamePair = std::pair<OrderStatus, const char*>;

        // OrderStatus <-> 대문자 영문 문자열 매핑 테이블(docs/PRD.md 5.4 PoC 호환 스키마).
        // Persistence/View 양쪽이 이 단일 테이블을 공유한다.
        constexpr std::array<OrderStatusNamePair, 5> kOrderStatusNames
        {
            OrderStatusNamePair{ OrderStatus::Reserved,  "RESERVED" },
            OrderStatusNamePair{ OrderStatus::Rejected,  "REJECTED" },
            OrderStatusNamePair{ OrderStatus::Producing, "PRODUCING" },
            OrderStatusNamePair{ OrderStatus::Confirmed, "CONFIRMED" },
            OrderStatusNamePair{ OrderStatus::Released,  "RELEASED" },
        };
    }

    bool IsValidOrderStatusTransition(OrderStatus from, OrderStatus to)
    {
        for (const auto& allowedTransition : kAllowedTransitions)
        {
            if (allowedTransition.first == from && allowedTransition.second == to)
            {
                return true;
            }
        }
        return false;
    }

    std::string OrderStatusToString(OrderStatus status)
    {
        for (const auto& entry : kOrderStatusNames)
        {
            if (entry.first == status)
            {
                return entry.second;
            }
        }
        throw std::invalid_argument("알 수 없는 OrderStatus 값입니다.");
    }

    OrderStatus OrderStatusFromString(const std::string& statusText)
    {
        for (const auto& entry : kOrderStatusNames)
        {
            if (statusText == entry.second)
            {
                return entry.first;
            }
        }
        return OrderStatus::Reserved; // 알 수 없는 값은 안전하게 초기 상태로 폴백한다.
    }
}
