#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../Model/Order.h"

namespace Persistence
{
    // 주문(Order) 데이터에 대한 CRUD 인터페이스. docs/PRD.md 5.2 / 5.4 참고.
    class IOrderRepository
    {
    public:
        virtual ~IOrderRepository() = default;

        // 이미 등록된 주문번호면 false를 반환하고 아무 것도 하지 않는다.
        virtual bool Create(const Model::Order& order) = 0;
        virtual std::vector<Model::Order> GetAll() const = 0;
        virtual std::optional<Model::Order> FindById(const std::string& orderId) const = 0;
        // 대상 주문번호가 없으면 false를 반환한다.
        virtual bool Update(const Model::Order& order) = 0;
        // 대상 주문번호가 없으면 false를 반환한다.
        virtual bool Remove(const std::string& orderId) = 0;
    };
}
