#pragma once

#include <string>
#include <vector>

#include "../Json/JsonValue.h"
#include "../Model/OrderStatus.h"
#include "IOrderRepository.h"

namespace Persistence
{
    // IOrderRepository의 JSON 파일 기반 구현체.
    // 생성자에서 파일을 읽어(Load) 메모리에 적재하고, 변경(Create/Update/Remove)될 때마다
    // 즉시 파일 전체를 다시 기록한다(Persist, write-through). 파일이 없거나 파싱에 실패하면
    // 예외를 던지지 않고 빈 목록으로 시작한다.
    // 파일 스키마는 docs/PRD.md 5.4절의 PoC 호환 스키마(orderId/sampleId/customerName/quantity/status)에
    // 시각 정산(Phase 6)을 위한 "createdAtEpochMillis" 필드를 추가한 형태를 따른다(기존 필드는 유지).
    class JsonOrderRepository : public IOrderRepository
    {
    public:
        explicit JsonOrderRepository(std::string filePath);

        bool Create(const Model::Order& order) override;
        std::vector<Model::Order> GetAll() const override;
        std::optional<Model::Order> FindById(const std::string& orderId) const override;
        bool Update(const Model::Order& order) override;
        bool Remove(const std::string& orderId) override;

    private:
        void Load();
        void Persist() const;

        static Json::Value ToJson(const Model::Order& order);
        static Model::Order FromJson(const Json::Value& json);

        static std::string OrderStatusToString(Model::OrderStatus status);
        static Model::OrderStatus OrderStatusFromString(const std::string& statusText);

        static long long ToEpochMilliseconds(Model::Order::TimePoint timePoint);
        static Model::Order::TimePoint FromEpochMilliseconds(long long epochMilliseconds);

        std::string filePath_;
        std::vector<Model::Order> orders_;
    };
}
