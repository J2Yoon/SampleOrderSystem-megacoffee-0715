#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <optional>
#include <string>
#include <vector>

#include "../src/Controller/SampleController.h"
#include "../src/Controller/ShipmentController.h"
#include "../src/Model/Order.h"
#include "../src/Model/OrderStatus.h"
#include "../src/Model/Sample.h"
#include "../src/Persistence/IOrderRepository.h"
#include "../src/Persistence/ISampleRepository.h"

using Controller::SampleController;
using Controller::ShipmentController;
using Controller::ShipmentResult;
using Model::Order;
using Model::OrderStatus;
using Model::Sample;

namespace
{
    // ISampleRepository의 인메모리 페이크.
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

    // IOrderRepository의 인메모리 페이크.
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

    class ShipmentControllerTest : public ::testing::Test
    {
    protected:
        InMemorySampleRepository sampleRepository_;
        InMemoryOrderRepository orderRepository_;

        SampleController MakeSampleController()
        {
            return SampleController(sampleRepository_);
        }

        void SeedSample(const std::string& sampleId, double averageProductionMinutesPerUnit,
            double yield, int stock)
        {
            sampleRepository_.Create(Sample(sampleId, "Sample-" + sampleId,
                averageProductionMinutesPerUnit, yield, stock));
        }

        void SeedOrder(const std::string& orderId, const std::string& sampleId,
            const std::string& customerName, int quantity, OrderStatus status)
        {
            orderRepository_.Create(Order(orderId, sampleId, customerName, quantity, status,
                MakeLocalDateTimePoint(2026, 7, 15)));
        }
    };

    // ---- GetShippableOrders ----

    TEST_F(ShipmentControllerTest, CONFIRMED_상태의_주문만_GetShippableOrders에_포함된다)
    {
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Confirmed);
        SeedOrder("ORD-2", "S001", "Customer-B", 10, OrderStatus::Reserved);
        SeedOrder("ORD-3", "S001", "Customer-C", 10, OrderStatus::Rejected);
        SeedOrder("ORD-4", "S001", "Customer-D", 10, OrderStatus::Producing);
        SeedOrder("ORD-5", "S001", "Customer-E", 10, OrderStatus::Released);
        auto sampleController = MakeSampleController();
        ShipmentController shipmentController(orderRepository_, sampleController);

        const auto shippableOrders = shipmentController.GetShippableOrders();

        ASSERT_EQ(shippableOrders.size(), 1u);
        EXPECT_EQ(shippableOrders[0].GetOrderId(), "ORD-1");
    }

    TEST_F(ShipmentControllerTest, CONFIRMED_주문이_없으면_GetShippableOrders는_빈_목록을_반환한다)
    {
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        ShipmentController shipmentController(orderRepository_, sampleController);

        EXPECT_TRUE(shipmentController.GetShippableOrders().empty());
    }

    // ---- ShipOrder: 정상 케이스 ----

    TEST_F(ShipmentControllerTest, CONFIRMED_주문을_출고하면_Success를_반환한다)
    {
        SeedSample("S001", 10.0, 0.9, 20);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Confirmed);
        auto sampleController = MakeSampleController();
        ShipmentController shipmentController(orderRepository_, sampleController);

        const auto result = shipmentController.ShipOrder("ORD-1");

        EXPECT_EQ(result, ShipmentResult::Success);
    }

    TEST_F(ShipmentControllerTest, CONFIRMED_주문을_출고하면_상태가_RELEASED로_전환된다)
    {
        SeedSample("S001", 10.0, 0.9, 20);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Confirmed);
        auto sampleController = MakeSampleController();
        ShipmentController shipmentController(orderRepository_, sampleController);

        shipmentController.ShipOrder("ORD-1");

        const auto updated = orderRepository_.FindById("ORD-1");
        ASSERT_TRUE(updated.has_value());
        EXPECT_EQ(updated->GetStatus(), OrderStatus::Released);
    }

    TEST_F(ShipmentControllerTest, 출고하면_재고가_주문수량만큼_정확히_감소한다)
    {
        SeedSample("S001", 10.0, 0.9, 20);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Confirmed);
        auto sampleController = MakeSampleController();
        ShipmentController shipmentController(orderRepository_, sampleController);

        shipmentController.ShipOrder("ORD-1");

        const auto sample = sampleRepository_.FindById("S001");
        ASSERT_TRUE(sample.has_value());
        EXPECT_EQ(sample->GetStock(), 10);
    }

    TEST_F(ShipmentControllerTest, 재고가_주문수량과_정확히_같아도_출고에_성공하고_재고는_0이_된다)
    {
        SeedSample("S001", 10.0, 0.9, 10);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Confirmed);
        auto sampleController = MakeSampleController();
        ShipmentController shipmentController(orderRepository_, sampleController);

        const auto result = shipmentController.ShipOrder("ORD-1");

        EXPECT_EQ(result, ShipmentResult::Success);
        const auto sample = sampleRepository_.FindById("S001");
        ASSERT_TRUE(sample.has_value());
        EXPECT_EQ(sample->GetStock(), 0);
    }

    TEST_F(ShipmentControllerTest, 부분출고는_지원하지_않으며_주문수량_전체가_한번에_출고된다)
    {
        // 재고가 주문수량보다 많아도 출고 수량은 항상 주문수량 전체이며 재고 전량이 아니다.
        SeedSample("S001", 10.0, 0.9, 50);
        SeedOrder("ORD-1", "S001", "Customer-A", 12, OrderStatus::Confirmed);
        auto sampleController = MakeSampleController();
        ShipmentController shipmentController(orderRepository_, sampleController);

        shipmentController.ShipOrder("ORD-1");

        const auto sample = sampleRepository_.FindById("S001");
        ASSERT_TRUE(sample.has_value());
        EXPECT_EQ(sample->GetStock(), 38);
    }

    // ---- ShipOrder: 존재하지 않는 주문 ----

    TEST_F(ShipmentControllerTest, 존재하지_않는_주문번호로_출고하면_OrderNotFound를_반환한다)
    {
        auto sampleController = MakeSampleController();
        ShipmentController shipmentController(orderRepository_, sampleController);

        const auto result = shipmentController.ShipOrder("NO-SUCH-ORDER");

        EXPECT_EQ(result, ShipmentResult::OrderNotFound);
    }

    // ---- ShipOrder: CONFIRMED가 아닌 상태 ----

    TEST_F(ShipmentControllerTest, RESERVED_상태의_주문을_출고하면_InvalidOrderState를_반환한다)
    {
        SeedSample("S001", 10.0, 0.9, 20);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        ShipmentController shipmentController(orderRepository_, sampleController);

        const auto result = shipmentController.ShipOrder("ORD-1");

        EXPECT_EQ(result, ShipmentResult::InvalidOrderState);
    }

    TEST_F(ShipmentControllerTest, PRODUCING_상태의_주문을_출고하면_InvalidOrderState를_반환한다)
    {
        SeedSample("S001", 10.0, 0.9, 20);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Producing);
        auto sampleController = MakeSampleController();
        ShipmentController shipmentController(orderRepository_, sampleController);

        const auto result = shipmentController.ShipOrder("ORD-1");

        EXPECT_EQ(result, ShipmentResult::InvalidOrderState);
    }

    TEST_F(ShipmentControllerTest, 이미_RELEASED인_주문을_다시_출고하면_InvalidOrderState를_반환한다)
    {
        SeedSample("S001", 10.0, 0.9, 20);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Released);
        auto sampleController = MakeSampleController();
        ShipmentController shipmentController(orderRepository_, sampleController);

        const auto result = shipmentController.ShipOrder("ORD-1");

        EXPECT_EQ(result, ShipmentResult::InvalidOrderState);
    }

    TEST_F(ShipmentControllerTest, REJECTED_상태의_주문을_출고하면_InvalidOrderState를_반환한다)
    {
        SeedSample("S001", 10.0, 0.9, 20);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Rejected);
        auto sampleController = MakeSampleController();
        ShipmentController shipmentController(orderRepository_, sampleController);

        const auto result = shipmentController.ShipOrder("ORD-1");

        EXPECT_EQ(result, ShipmentResult::InvalidOrderState);
    }

    TEST_F(ShipmentControllerTest, CONFIRMED가_아닌_주문에_출고시도해도_상태는_바뀌지_않는다)
    {
        SeedSample("S001", 10.0, 0.9, 20);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        ShipmentController shipmentController(orderRepository_, sampleController);

        shipmentController.ShipOrder("ORD-1");

        const auto updated = orderRepository_.FindById("ORD-1");
        ASSERT_TRUE(updated.has_value());
        EXPECT_EQ(updated->GetStatus(), OrderStatus::Reserved);
    }

    TEST_F(ShipmentControllerTest, CONFIRMED가_아닌_주문에_출고시도해도_재고는_변경되지_않는다)
    {
        SeedSample("S001", 10.0, 0.9, 20);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Producing);
        auto sampleController = MakeSampleController();
        ShipmentController shipmentController(orderRepository_, sampleController);

        shipmentController.ShipOrder("ORD-1");

        const auto sample = sampleRepository_.FindById("S001");
        ASSERT_TRUE(sample.has_value());
        EXPECT_EQ(sample->GetStock(), 20);
    }

    // ---- ShipOrder: 시료를 찾을 수 없는 방어적 케이스 ----

    TEST_F(ShipmentControllerTest, 주문이_참조하는_시료가_존재하지_않으면_SampleNotFound를_반환한다)
    {
        SeedOrder("ORD-1", "MISSING-SAMPLE", "Customer-A", 10, OrderStatus::Confirmed);
        auto sampleController = MakeSampleController();
        ShipmentController shipmentController(orderRepository_, sampleController);

        const auto result = shipmentController.ShipOrder("ORD-1");

        EXPECT_EQ(result, ShipmentResult::SampleNotFound);
    }

    TEST_F(ShipmentControllerTest, 시료를_찾지_못해_SampleNotFound가_되면_주문상태는_CONFIRMED로_유지된다)
    {
        SeedOrder("ORD-1", "MISSING-SAMPLE", "Customer-A", 10, OrderStatus::Confirmed);
        auto sampleController = MakeSampleController();
        ShipmentController shipmentController(orderRepository_, sampleController);

        shipmentController.ShipOrder("ORD-1");

        const auto updated = orderRepository_.FindById("ORD-1");
        ASSERT_TRUE(updated.has_value());
        EXPECT_EQ(updated->GetStatus(), OrderStatus::Confirmed);
    }
}
