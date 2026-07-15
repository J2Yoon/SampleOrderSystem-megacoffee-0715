#pragma once

#include <chrono>
#include <string>
#include <vector>

#include "../Model/Order.h"
#include "../Model/ProductionQueueItem.h"
#include "../Model/Sample.h"
#include "../Persistence/IOrderRepository.h"
#include "../Persistence/IProductionQueueRepository.h"
#include "SampleController.h"

namespace Controller
{
    // 주문 승인 처리 결과. 실패 원인을 구분해 View가 적절한 안내 메시지를 표시할 수 있게 한다.
    enum class OrderApprovalResult
    {
        Success,
        OrderNotFound,      // 존재하지 않는 주문번호
        InvalidOrderState,  // RESERVED 상태가 아니어서 승인할 수 없음
        SampleNotFound      // 주문이 참조하는 시료를 더 이상 찾을 수 없음(방어적 처리)
    };

    // 주문 거절 처리 결과.
    enum class OrderRejectionResult
    {
        Success,
        OrderNotFound,      // 존재하지 않는 주문번호
        InvalidOrderState   // 이미 REJECTED 등 종단 상태거나 그 외 거절 불가 상태
    };

    // 주문 승인/거절(재고 판단 로직) 기능을 담당하는 Controller. docs/PRD.md 4.4 참고.
    // 콘솔 입출력(<iostream>)에 의존하지 않는다. 재고 조회는 SampleController에 위임하며,
    // 승인 시점에는 재고를 물리적으로 차감하지 않는다(docs/PRD.md 4.6.1).
    class OrderApprovalController
    {
    public:
        OrderApprovalController(
            Persistence::IOrderRepository& orderRepository,
            const SampleController& sampleController,
            Persistence::IProductionQueueRepository& productionQueueRepository);

        // RESERVED 상태의 주문만 필터링해 반환한다(docs/PRD.md 4.4.1).
        std::vector<Model::Order> GetReservedOrders() const;

        // 재고 조회 결과에 따라 CONFIRMED(충분) 또는 PRODUCING(부족 + 생산 큐 등록)으로 전환한다.
        // approvedAt을 생략하면 호출 시점의 현재 시각을 사용한다.
        OrderApprovalResult ApproveOrder(
            const std::string& orderId,
            Model::Order::TimePoint approvedAt = std::chrono::system_clock::now());

        // 주문을 REJECTED로 전환한다(종단 상태, 이후 전이는 차단됨).
        OrderRejectionResult RejectOrder(const std::string& orderId);

    private:
        // 승인 시점에 파생되는 생산 큐 항목을 생성한다(Factory Method — CLAUDE.md Clean Code 원칙 참고).
        // 실 생산량/총 생산 시간 계산식은 ProductionCalculator(ProductionLineController와 공유)에 위임한다.
        static Model::ProductionQueueItem CreateProductionQueueItem(
            const Model::Order& order,
            const Model::Sample& sample,
            int shortageQuantity,
            Model::Order::TimePoint approvedAt);

        Persistence::IOrderRepository& orderRepository_;
        const SampleController& sampleController_;
        Persistence::IProductionQueueRepository& productionQueueRepository_;
    };
}
