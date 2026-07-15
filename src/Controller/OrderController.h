#pragma once

#include <chrono>
#include <string>
#include <vector>

#include "../Model/Order.h"
#include "../Persistence/IOrderRepository.h"
#include "SampleController.h"

namespace Controller
{
    // 주문 접수 결과. 실패 원인을 구분해 View가 적절한 안내 메시지를 표시할 수 있게 한다.
    enum class OrderPlacementResult
    {
        Success,
        UnregisteredSample,  // 시스템에 등록되지 않은 시료 ID(docs/PRD.md 4.2)
        InvalidInput         // 고객명이 비어있거나 주문 수량이 0 이하
    };

    // 주문 접수(예약) 기능을 담당하는 Controller. docs/PRD.md 4.3 참고.
    // 콘솔 입출력(<iostream>)에 의존하지 않으며, 등록되지 않은 시료 검증은 SampleController에 위임한다.
    class OrderController
    {
    public:
        OrderController(
            Persistence::IOrderRepository& orderRepository,
            const SampleController& sampleController);

        // 새 주문을 RESERVED 상태로 접수한다. 재고는 물리적으로 차감하지 않는다(docs/PRD.md 4.6.1).
        // createdAt을 생략하면 호출 시점의 현재 시각을 사용한다.
        OrderPlacementResult PlaceOrder(
            const std::string& sampleId,
            const std::string& customerName,
            int quantity,
            Model::Order::TimePoint createdAt = std::chrono::system_clock::now());

        // 접수된 모든 주문(상태 무관)을 조회한다.
        std::vector<Model::Order> GetAllOrders() const;

    private:
        static bool IsValidOrderInput(const std::string& customerName, int quantity);

        // "ORD-YYYYMMDD-XXXX" 포맷의 주문번호를 생성한다(docs/PRD.md 5.2, docs_temp/phase_4.md 설계 결정 2).
        std::string GenerateOrderId(Model::Order::TimePoint createdAt) const;

        static std::string FormatDateSegment(Model::Order::TimePoint timePoint);
        static int FindNextDailySequenceNumber(
            const std::vector<Model::Order>& existingOrders,
            const std::string& orderIdDatePrefix);

        Persistence::IOrderRepository& orderRepository_;
        const SampleController& sampleController_;
    };
}
