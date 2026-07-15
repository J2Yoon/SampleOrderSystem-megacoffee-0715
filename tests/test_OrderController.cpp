#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <optional>
#include <string>
#include <vector>

#include "../src/Controller/OrderController.h"
#include "../src/Controller/SampleController.h"
#include "../src/Persistence/IOrderRepository.h"
#include "../src/Persistence/ISampleRepository.h"

using Controller::OrderController;
using Controller::OrderPlacementResult;
using Controller::SampleController;
using Model::Order;
using Model::Sample;

namespace
{
    // ISampleRepository의 인메모리 페이크. 실제 파일 I/O 없이 로직만 검증하기 위함.
    class InMemorySampleRepository : public Persistence::ISampleRepository
    {
    public:
        bool Create(const Sample& sample) override
        {
            if (FindById(sample.GetSampleId()).has_value())
            {
                return false;
            }
            samples_.push_back(sample);
            return true;
        }

        std::vector<Sample> GetAll() const override
        {
            return samples_;
        }

        std::optional<Sample> FindById(const std::string& sampleId) const override
        {
            const auto iterator = std::find_if(samples_.begin(), samples_.end(),
                [&sampleId](const Sample& sample) { return sample.GetSampleId() == sampleId; });
            if (iterator == samples_.end())
            {
                return std::nullopt;
            }
            return *iterator;
        }

        bool Update(const Sample& sample) override
        {
            for (auto& existing : samples_)
            {
                if (existing.GetSampleId() == sample.GetSampleId())
                {
                    existing = sample;
                    return true;
                }
            }
            return false;
        }

        bool Remove(const std::string& sampleId) override
        {
            const auto iterator = std::find_if(samples_.begin(), samples_.end(),
                [&sampleId](const Sample& sample) { return sample.GetSampleId() == sampleId; });
            if (iterator == samples_.end())
            {
                return false;
            }
            samples_.erase(iterator);
            return true;
        }

    private:
        std::vector<Sample> samples_;
    };

    // IOrderRepository의 인메모리 페이크. 실제 파일 I/O 없이 OrderController 로직만 검증하기 위함.
    class InMemoryOrderRepository : public Persistence::IOrderRepository
    {
    public:
        bool Create(const Order& order) override
        {
            if (FindById(order.GetOrderId()).has_value())
            {
                return false;
            }
            orders_.push_back(order);
            return true;
        }

        std::vector<Order> GetAll() const override
        {
            return orders_;
        }

        std::optional<Order> FindById(const std::string& orderId) const override
        {
            const auto iterator = std::find_if(orders_.begin(), orders_.end(),
                [&orderId](const Order& order) { return order.GetOrderId() == orderId; });
            if (iterator == orders_.end())
            {
                return std::nullopt;
            }
            return *iterator;
        }

        bool Update(const Order& order) override
        {
            for (auto& existing : orders_)
            {
                if (existing.GetOrderId() == order.GetOrderId())
                {
                    existing = order;
                    return true;
                }
            }
            return false;
        }

        bool Remove(const std::string& orderId) override
        {
            const auto iterator = std::find_if(orders_.begin(), orders_.end(),
                [&orderId](const Order& order) { return order.GetOrderId() == orderId; });
            if (iterator == orders_.end())
            {
                return false;
            }
            orders_.erase(iterator);
            return true;
        }

    private:
        std::vector<Order> orders_;
    };

    // 특정 로컬 날짜(YYYY, MM, DD)의 정오 시각으로 TimePoint를 만든다.
    // 자정 부근 타임존 경계 이슈를 피하기 위해 정오를 사용한다.
    Order::TimePoint MakeLocalDateTimePoint(int year, int month, int day)
    {
        std::tm timeStruct{};
        timeStruct.tm_year = year - 1900;
        timeStruct.tm_mon = month - 1;
        timeStruct.tm_mday = day;
        timeStruct.tm_hour = 12;
        timeStruct.tm_min = 0;
        timeStruct.tm_sec = 0;
        timeStruct.tm_isdst = -1;
        const std::time_t time = std::mktime(&timeStruct);
        return std::chrono::system_clock::from_time_t(time);
    }

    class OrderControllerTest : public ::testing::Test
    {
    protected:
        InMemorySampleRepository sampleRepository_;
        InMemoryOrderRepository orderRepository_;
    };

    // ---- 정상 접수 ----

    TEST_F(OrderControllerTest, 등록된_시료로_유효한_입력이면_Success를_반환한다)
    {
        SampleController sampleController(sampleRepository_);
        sampleController.RegisterSample("S001", "Sample-A", 12.5, 0.9);
        OrderController orderController(orderRepository_, sampleController);

        const auto result = orderController.PlaceOrder("S001", "Customer-A", 10);

        EXPECT_EQ(result, OrderPlacementResult::Success);
    }

    TEST_F(OrderControllerTest, 접수에_성공하면_저장소에_주문이_추가된다)
    {
        SampleController sampleController(sampleRepository_);
        sampleController.RegisterSample("S001", "Sample-A", 12.5, 0.9);
        OrderController orderController(orderRepository_, sampleController);

        orderController.PlaceOrder("S001", "Customer-A", 10);

        const auto all = orderRepository_.GetAll();
        ASSERT_EQ(all.size(), 1u);
        EXPECT_EQ(all[0].GetSampleId(), "S001");
        EXPECT_EQ(all[0].GetCustomerName(), "Customer-A");
        EXPECT_EQ(all[0].GetQuantity(), 10);
    }

    TEST_F(OrderControllerTest, 접수에_성공하면_주문_상태는_RESERVED이다)
    {
        SampleController sampleController(sampleRepository_);
        sampleController.RegisterSample("S001", "Sample-A", 12.5, 0.9);
        OrderController orderController(orderRepository_, sampleController);

        orderController.PlaceOrder("S001", "Customer-A", 10);

        const auto all = orderRepository_.GetAll();
        ASSERT_EQ(all.size(), 1u);
        EXPECT_EQ(all[0].GetStatus(), Model::OrderStatus::Reserved);
    }

    // ---- 미등록 시료 ----

    TEST_F(OrderControllerTest, 미등록_시료ID로_주문하면_UnregisteredSample을_반환한다)
    {
        SampleController sampleController(sampleRepository_);
        OrderController orderController(orderRepository_, sampleController);

        const auto result = orderController.PlaceOrder("UNKNOWN", "Customer-A", 10);

        EXPECT_EQ(result, OrderPlacementResult::UnregisteredSample);
    }

    TEST_F(OrderControllerTest, 미등록_시료ID로_주문하면_저장소에_추가되지_않는다)
    {
        SampleController sampleController(sampleRepository_);
        OrderController orderController(orderRepository_, sampleController);

        orderController.PlaceOrder("UNKNOWN", "Customer-A", 10);

        EXPECT_TRUE(orderRepository_.GetAll().empty());
    }

    // ---- 입력 검증 실패 ----

    TEST_F(OrderControllerTest, 고객명이_비어있으면_InvalidInput을_반환한다)
    {
        SampleController sampleController(sampleRepository_);
        sampleController.RegisterSample("S001", "Sample-A", 12.5, 0.9);
        OrderController orderController(orderRepository_, sampleController);

        const auto result = orderController.PlaceOrder("S001", "", 10);

        EXPECT_EQ(result, OrderPlacementResult::InvalidInput);
    }

    TEST_F(OrderControllerTest, 수량이_0이면_InvalidInput을_반환한다)
    {
        SampleController sampleController(sampleRepository_);
        sampleController.RegisterSample("S001", "Sample-A", 12.5, 0.9);
        OrderController orderController(orderRepository_, sampleController);

        const auto result = orderController.PlaceOrder("S001", "Customer-A", 0);

        EXPECT_EQ(result, OrderPlacementResult::InvalidInput);
    }

    TEST_F(OrderControllerTest, 수량이_음수면_InvalidInput을_반환한다)
    {
        SampleController sampleController(sampleRepository_);
        sampleController.RegisterSample("S001", "Sample-A", 12.5, 0.9);
        OrderController orderController(orderRepository_, sampleController);

        const auto result = orderController.PlaceOrder("S001", "Customer-A", -5);

        EXPECT_EQ(result, OrderPlacementResult::InvalidInput);
    }

    TEST_F(OrderControllerTest, 입력_검증에_실패하면_저장소에_추가되지_않는다)
    {
        SampleController sampleController(sampleRepository_);
        sampleController.RegisterSample("S001", "Sample-A", 12.5, 0.9);
        OrderController orderController(orderRepository_, sampleController);

        orderController.PlaceOrder("S001", "", 0);

        EXPECT_TRUE(orderRepository_.GetAll().empty());
    }

    TEST_F(OrderControllerTest, 미등록_시료와_잘못된_입력이_동시일_때_UnregisteredSample이_우선한다)
    {
        SampleController sampleController(sampleRepository_);
        OrderController orderController(orderRepository_, sampleController);

        const auto result = orderController.PlaceOrder("UNKNOWN", "", -1);

        EXPECT_EQ(result, OrderPlacementResult::UnregisteredSample);
    }

    // ---- 주문번호 포맷/순번 ----

    TEST_F(OrderControllerTest, 고정된_날짜에_첫_주문이면_순번_0001로_생성된다)
    {
        SampleController sampleController(sampleRepository_);
        sampleController.RegisterSample("S001", "Sample-A", 12.5, 0.9);
        OrderController orderController(orderRepository_, sampleController);
        const auto createdAt = MakeLocalDateTimePoint(2026, 7, 15);

        orderController.PlaceOrder("S001", "Customer-A", 10, createdAt);

        const auto all = orderRepository_.GetAll();
        ASSERT_EQ(all.size(), 1u);
        EXPECT_EQ(all[0].GetOrderId(), "ORD-20260715-0001");
    }

    TEST_F(OrderControllerTest, 같은_날짜에_두번째_주문이면_순번이_0002로_증가한다)
    {
        SampleController sampleController(sampleRepository_);
        sampleController.RegisterSample("S001", "Sample-A", 12.5, 0.9);
        OrderController orderController(orderRepository_, sampleController);
        const auto createdAt = MakeLocalDateTimePoint(2026, 7, 15);
        orderController.PlaceOrder("S001", "Customer-A", 10, createdAt);

        orderController.PlaceOrder("S001", "Customer-B", 5, createdAt);

        const auto all = orderRepository_.GetAll();
        ASSERT_EQ(all.size(), 2u);
        EXPECT_EQ(all[1].GetOrderId(), "ORD-20260715-0002");
    }

    TEST_F(OrderControllerTest, 날짜가_다르면_순번이_0001로_리셋된다)
    {
        SampleController sampleController(sampleRepository_);
        sampleController.RegisterSample("S001", "Sample-A", 12.5, 0.9);
        OrderController orderController(orderRepository_, sampleController);
        orderController.PlaceOrder("S001", "Customer-A", 10, MakeLocalDateTimePoint(2026, 7, 15));

        orderController.PlaceOrder("S001", "Customer-B", 5, MakeLocalDateTimePoint(2026, 7, 16));

        const auto all = orderRepository_.GetAll();
        ASSERT_EQ(all.size(), 2u);
        EXPECT_EQ(all[1].GetOrderId(), "ORD-20260716-0001");
    }

    TEST_F(OrderControllerTest, 기존_주문이_0003까지_있으면_다음_순번은_0004이다)
    {
        SampleController sampleController(sampleRepository_);
        sampleController.RegisterSample("S001", "Sample-A", 12.5, 0.9);
        const auto createdAt = MakeLocalDateTimePoint(2026, 7, 15);
        orderRepository_.Create(Order("ORD-20260715-0001", "S001", "Customer-X", 1, createdAt));
        orderRepository_.Create(Order("ORD-20260715-0002", "S001", "Customer-Y", 1, createdAt));
        orderRepository_.Create(Order("ORD-20260715-0003", "S001", "Customer-Z", 1, createdAt));
        OrderController orderController(orderRepository_, sampleController);

        orderController.PlaceOrder("S001", "Customer-A", 10, createdAt);

        const auto all = orderRepository_.GetAll();
        ASSERT_EQ(all.size(), 4u);
        EXPECT_EQ(all[3].GetOrderId(), "ORD-20260715-0004");
    }

    // ---- 전체 조회 ----

    TEST_F(OrderControllerTest, 주문이_없으면_GetAllOrders는_빈_목록을_반환한다)
    {
        SampleController sampleController(sampleRepository_);
        OrderController orderController(orderRepository_, sampleController);

        EXPECT_TRUE(orderController.GetAllOrders().empty());
    }

    TEST_F(OrderControllerTest, 여러_주문을_접수하면_GetAllOrders는_접수한_개수만큼_반환한다)
    {
        SampleController sampleController(sampleRepository_);
        sampleController.RegisterSample("S001", "Sample-A", 12.5, 0.9);
        OrderController orderController(orderRepository_, sampleController);
        const auto createdAt = MakeLocalDateTimePoint(2026, 7, 15);

        orderController.PlaceOrder("S001", "Customer-A", 10, createdAt);
        orderController.PlaceOrder("S001", "Customer-B", 5, createdAt);

        EXPECT_EQ(orderController.GetAllOrders().size(), 2u);
    }
}
