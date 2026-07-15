#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../Model/ProductionQueueItem.h"

namespace Persistence
{
    // 생산 큐 항목(ProductionQueueItem) 데이터에 대한 CRUD 인터페이스. docs/PRD.md 5.3 참고.
    // 연결 주문번호(orderId)가 항목의 고유 키 역할을 한다(주문 1건당 생산 작업 1건, PRD 4.4.2 — 병합 금지).
    class IProductionQueueRepository
    {
    public:
        virtual ~IProductionQueueRepository() = default;

        // 이미 동일 주문번호의 항목이 있으면 false를 반환하고 아무 것도 하지 않는다.
        virtual bool Create(const Model::ProductionQueueItem& item) = 0;
        // FIFO 처리를 위해 큐 진입 순서(파일에 기록된 순서)를 그대로 반환한다.
        virtual std::vector<Model::ProductionQueueItem> GetAll() const = 0;
        virtual std::optional<Model::ProductionQueueItem> FindById(const std::string& orderId) const = 0;
        // 대상 항목이 없으면 false를 반환한다.
        virtual bool Update(const Model::ProductionQueueItem& item) = 0;
        // 대상 항목이 없으면 false를 반환한다(생산 완료 후 큐에서 제거하는 용도).
        virtual bool Remove(const std::string& orderId) = 0;
    };
}
