#pragma once

#include <chrono>
#include <string>

#include "OrderStatus.h"

namespace Model
{
    // 주문(Order) — docs/PRD.md 3절 / 5.2 참고
    // 순수 데이터 모델이며 콘솔 입출력, 파일 I/O에 의존하지 않는다.
    class Order
    {
    public:
        using TimePoint = std::chrono::system_clock::time_point;

        // 신규 주문 접수용 생성자. 상태는 항상 RESERVED로 시작한다(docs/PRD.md 4.3.1).
        Order(
            std::string orderId,
            std::string sampleId,
            std::string customerName,
            int quantity,
            TimePoint createdAt);

        // 영속성 복원 등 임의 상태를 그대로 지정해야 하는 경우를 위한 생성자.
        Order(
            std::string orderId,
            std::string sampleId,
            std::string customerName,
            int quantity,
            OrderStatus status,
            TimePoint createdAt);

        const std::string& GetOrderId() const;
        const std::string& GetSampleId() const;
        const std::string& GetCustomerName() const;
        int GetQuantity() const;
        OrderStatus GetStatus() const;
        TimePoint GetCreatedAt() const;

        // 유효한 상태 전이일 때만 상태를 변경하고 true를 반환한다.
        // 유효하지 않은 전이(예: REJECTED 이후 재전이)는 상태를 바꾸지 않고 false를 반환한다.
        bool TryTransitionTo(OrderStatus newStatus);

    private:
        std::string orderId_;
        std::string sampleId_;
        std::string customerName_;
        int quantity_;
        OrderStatus status_;
        TimePoint createdAt_;
    };
}
