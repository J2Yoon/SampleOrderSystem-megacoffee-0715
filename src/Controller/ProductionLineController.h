#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "../Model/ProductionQueueItem.h"
#include "../Persistence/IOrderRepository.h"
#include "../Persistence/IProductionQueueRepository.h"
#include "SampleController.h"

namespace Controller
{
    // 생산 라인의 특정 큐 항목이 단일 FIFO 라인에서 실제로 언제 시작/완료되는지를 나타내는 값 객체.
    // ProductionQueueItem이 보유한 productionStartTime/expectedCompletionTime은 승인(큐 진입) 시각을
    // 기준으로 계산된 값일 뿐이며, 앞선 항목이 아직 진행 중이면 실제 시작은 더 늦어진다(docs/PRD.md 4.6).
    struct ProductionLineStatus
    {
        Model::ProductionQueueItem item;
        Model::ProductionQueueItem::TimePoint effectiveStartTime;
        Model::ProductionQueueItem::TimePoint effectiveCompletionTime;
    };

    // 생산 라인(단일 라인, FIFO 큐) 제어를 담당하는 Controller. docs/PRD.md 4.6 참고.
    // 콘솔 입출력(<iostream>)에 의존하지 않는다.
    class ProductionLineController
    {
    public:
        ProductionLineController(
            Persistence::IProductionQueueRepository& productionQueueRepository,
            Persistence::IOrderRepository& orderRepository,
            SampleController& sampleController);

        // now 시점까지 실제로 완료됐어야 할 큐 항목을 FIFO 순서대로 모두 정산한다.
        // 정산 = 재고를 실 생산량만큼 증가 + 연결 주문을 PRODUCING -> CONFIRMED로 전환 + 큐에서 제거.
        // 앱 시작(재시작 포함) 시 반드시 먼저 호출해야 그동안 흐른 실제 경과 시간이 반영된다(docs/PRD.md 4.6.4).
        // 정산된 항목 수를 반환한다.
        int SettleCompletedItems(
            Model::ProductionQueueItem::TimePoint now = std::chrono::system_clock::now());

        // 현재 생산 중(큐의 맨 앞)인 항목의 실제 시작/완료 시각을 반환한다. 큐가 비어 있으면 nullopt.
        // 정산되지 않은 완료 항목이 남아있을 수 있으므로 SettleCompletedItems 호출 이후 사용을 전제로 한다.
        std::optional<ProductionLineStatus> GetCurrentProductionStatus() const;

        // 현재 생산 중인 항목을 제외한 나머지 대기 큐를 FIFO 순서 그대로 반환한다.
        std::vector<Model::ProductionQueueItem> GetWaitingQueue() const;

        // 큐에 남아있는 전체 항목 수(대기 중 + 현재 생산 중)를 반환한다. 메인 메뉴 요약 정보 등에서
        // 사용하며, SettleCompletedItems 호출 이후(완료된 항목이 이미 제거된 상태) 사용을 전제로 한다.
        int GetPendingItemCount() const;

    private:
        // 단일 라인은 한 번에 하나씩만 생산하므로, 앞선 항목이 끝나야 다음 항목이 시작될 수 있다는
        // 제약을 반영해 큐 전체의 실제 시작/완료 시각을 FIFO 순서대로 누적 계산한다.
        std::vector<ProductionLineStatus> BuildSchedule() const;

        // 완료된 큐 항목 하나를 정산 처리(재고 증가 + 상태 전환 + 큐 제거)한다.
        void CompleteProductionQueueItem(const Model::ProductionQueueItem& item);

        Persistence::IProductionQueueRepository& productionQueueRepository_;
        Persistence::IOrderRepository& orderRepository_;
        SampleController& sampleController_;
    };
}
