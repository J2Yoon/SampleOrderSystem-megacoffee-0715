#pragma once

#include <string>
#include <vector>

#include "../Json/JsonValue.h"
#include "IProductionQueueRepository.h"

namespace Persistence
{
    // IProductionQueueRepository의 JSON 파일 기반 구현체.
    // 생성자에서 파일을 읽어(Load) 메모리에 적재하고, 변경(Create/Update/Remove)될 때마다
    // 즉시 파일 전체를 다시 기록한다(Persist, write-through). 파일이 없거나 파싱에 실패하면
    // 예외를 던지지 않고 빈 목록으로 시작한다.
    //
    // 저장 파일: data/productionQueue.json (docs_temp/phase_2.md 결정 사항 — samples.json/orders.json과
    // 달리 PoC 호환 대상이 아니므로 자유롭게 설계함). 필드: orderId, shortageQuantity,
    // actualProductionQuantity, totalProductionMinutes, productionStartEpochMillis,
    // expectedCompletionEpochMillis(참고용 파생값, 로드 시에는 사용하지 않고 항상 재계산됨).
    class JsonProductionQueueRepository : public IProductionQueueRepository
    {
    public:
        explicit JsonProductionQueueRepository(std::string filePath);

        bool Create(const Model::ProductionQueueItem& item) override;
        std::vector<Model::ProductionQueueItem> GetAll() const override;
        std::optional<Model::ProductionQueueItem> FindById(const std::string& orderId) const override;
        bool Update(const Model::ProductionQueueItem& item) override;
        bool Remove(const std::string& orderId) override;

    private:
        void Load();
        void Persist() const;

        static Json::Value ToJson(const Model::ProductionQueueItem& item);
        static Model::ProductionQueueItem FromJson(const Json::Value& json);

        static long long ToEpochMilliseconds(Model::ProductionQueueItem::TimePoint timePoint);
        static Model::ProductionQueueItem::TimePoint FromEpochMilliseconds(long long epochMilliseconds);

        std::string filePath_;
        std::vector<Model::ProductionQueueItem> items_;
    };
}
