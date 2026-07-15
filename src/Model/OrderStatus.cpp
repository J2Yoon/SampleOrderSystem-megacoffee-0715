#include "OrderStatus.h"

#include <array>
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
}
