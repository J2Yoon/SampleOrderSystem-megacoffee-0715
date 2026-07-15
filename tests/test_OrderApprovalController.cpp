#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <optional>
#include <string>
#include <vector>

#include "../src/Controller/OrderApprovalController.h"
#include "../src/Controller/SampleController.h"
#include "../src/Model/Order.h"
#include "../src/Model/OrderStatus.h"
#include "../src/Model/ProductionQueueItem.h"
#include "../src/Model/Sample.h"
#include "../src/Persistence/IOrderRepository.h"
#include "../src/Persistence/IProductionQueueRepository.h"
#include "../src/Persistence/ISampleRepository.h"

using Controller::OrderApprovalController;
using Controller::OrderApprovalResult;
using Controller::OrderRejectionResult;
using Controller::SampleController;
using Model::Order;
using Model::OrderStatus;
using Model::ProductionQueueItem;
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

    // IProductionQueueRepository의 인메모리 페이크. 주문번호가 키이므로 중복 생성을 막는다(병합 금지 검증용).
    class InMemoryProductionQueueRepository : public Persistence::IProductionQueueRepository
    {
    public:
        bool Create(const ProductionQueueItem& item) override
        {
            if (FindById(item.GetOrderId()).has_value())
            {
                return false;
            }
            items_.push_back(item);
            return true;
        }

        std::vector<ProductionQueueItem> GetAll() const override
        {
            return items_;
        }

        std::optional<ProductionQueueItem> FindById(const std::string& orderId) const override
        {
            const auto iterator = std::find_if(items_.begin(), items_.end(),
                [&orderId](const ProductionQueueItem& item) { return item.GetOrderId() == orderId; });
            if (iterator == items_.end())
            {
                return std::nullopt;
            }
            return *iterator;
        }

        bool Update(const ProductionQueueItem& item) override
        {
            for (auto& existing : items_)
            {
                if (existing.GetOrderId() == item.GetOrderId())
                {
                    existing = item;
                    return true;
                }
            }
            return false;
        }

        bool Remove(const std::string& orderId) override
        {
            const auto iterator = std::find_if(items_.begin(), items_.end(),
                [&orderId](const ProductionQueueItem& item) { return item.GetOrderId() == orderId; });
            if (iterator == items_.end())
            {
                return false;
            }
            items_.erase(iterator);
            return true;
        }

    private:
        std::vector<ProductionQueueItem> items_;
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

    class OrderApprovalControllerTest : public ::testing::Test
    {
    protected:
        InMemorySampleRepository sampleRepository_;
        InMemoryOrderRepository orderRepository_;
        InMemoryProductionQueueRepository productionQueueRepository_;

        SampleController MakeSampleController()
        {
            return SampleController(sampleRepository_);
        }

        // 시료를 직접 저장소에 등록하고(재고 포함) 반환한다. RegisterSample은 초기 재고를 항상 0으로
        // 고정하므로, 임의 재고값을 세팅하려면 저장소에 직접 넣어야 한다.
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

    // ---- GetReservedOrders ----

    TEST_F(OrderApprovalControllerTest, RESERVED_상태의_주문만_GetReservedOrders에_포함된다)
    {
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        SeedOrder("ORD-2", "S001", "Customer-B", 10, OrderStatus::Rejected);
        SeedOrder("ORD-3", "S001", "Customer-C", 10, OrderStatus::Producing);
        SeedOrder("ORD-4", "S001", "Customer-D", 10, OrderStatus::Confirmed);
        SeedOrder("ORD-5", "S001", "Customer-E", 10, OrderStatus::Released);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        const auto reservedOrders = approvalController.GetReservedOrders();

        ASSERT_EQ(reservedOrders.size(), 1u);
        EXPECT_EQ(reservedOrders[0].GetOrderId(), "ORD-1");
    }

    TEST_F(OrderApprovalControllerTest, RESERVED_주문이_없으면_GetReservedOrders는_빈_목록을_반환한다)
    {
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Confirmed);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        EXPECT_TRUE(approvalController.GetReservedOrders().empty());
    }

    // ---- ApproveOrder: 재고 충분 ----

    TEST_F(OrderApprovalControllerTest, 재고가_주문수량보다_많으면_승인시_Success를_반환한다)
    {
        SeedSample("S001", 10.0, 0.9, 20);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        const auto result = approvalController.ApproveOrder("ORD-1");

        EXPECT_EQ(result, OrderApprovalResult::Success);
    }

    TEST_F(OrderApprovalControllerTest, 재고가_주문수량보다_많으면_승인후_주문상태는_CONFIRMED이다)
    {
        SeedSample("S001", 10.0, 0.9, 20);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.ApproveOrder("ORD-1");

        const auto updated = orderRepository_.FindById("ORD-1");
        ASSERT_TRUE(updated.has_value());
        EXPECT_EQ(updated->GetStatus(), OrderStatus::Confirmed);
    }

    TEST_F(OrderApprovalControllerTest, 재고가_주문수량과_정확히_같으면_승인후_주문상태는_CONFIRMED이다)
    {
        SeedSample("S001", 10.0, 0.9, 10);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.ApproveOrder("ORD-1");

        const auto updated = orderRepository_.FindById("ORD-1");
        ASSERT_TRUE(updated.has_value());
        EXPECT_EQ(updated->GetStatus(), OrderStatus::Confirmed);
    }

    TEST_F(OrderApprovalControllerTest, 재고가_주문수량과_정확히_같으면_생산큐에_항목이_추가되지_않는다)
    {
        SeedSample("S001", 10.0, 0.9, 10);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.ApproveOrder("ORD-1");

        EXPECT_TRUE(productionQueueRepository_.GetAll().empty());
    }

    TEST_F(OrderApprovalControllerTest, 재고가_충분하면_생산큐에_항목이_추가되지_않는다)
    {
        SeedSample("S001", 10.0, 0.9, 20);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.ApproveOrder("ORD-1");

        EXPECT_TRUE(productionQueueRepository_.GetAll().empty());
    }

    TEST_F(OrderApprovalControllerTest, 재고가_고갈되어도_주문수량이_0이면_승인후_CONFIRMED이다)
    {
        // 재고 0, 재고 0 기준 부족분 계산 시 주문수량이 재고 이하가 되는 경계 확인용(주문수량 0은 입력
        // 검증 계층(OrderController)에서 차단되지만, 승인 로직 자체는 유효한 값만 받는다는 전제를 확인한다.
        SeedSample("S001", 10.0, 0.9, 0);
        SeedOrder("ORD-1", "S001", "Customer-A", 0, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.ApproveOrder("ORD-1");

        const auto updated = orderRepository_.FindById("ORD-1");
        ASSERT_TRUE(updated.has_value());
        EXPECT_EQ(updated->GetStatus(), OrderStatus::Confirmed);
    }

    // ---- ApproveOrder: 재고 부족 ----

    TEST_F(OrderApprovalControllerTest, 재고가_부족하면_승인후_주문상태는_PRODUCING이다)
    {
        SeedSample("S001", 10.0, 0.9, 3);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.ApproveOrder("ORD-1");

        const auto updated = orderRepository_.FindById("ORD-1");
        ASSERT_TRUE(updated.has_value());
        EXPECT_EQ(updated->GetStatus(), OrderStatus::Producing);
    }

    TEST_F(OrderApprovalControllerTest, 재고가_부족하면_생산큐에_정확히_1건_등록된다)
    {
        SeedSample("S001", 10.0, 0.9, 3);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.ApproveOrder("ORD-1");

        EXPECT_EQ(productionQueueRepository_.GetAll().size(), 1u);
    }

    TEST_F(OrderApprovalControllerTest, 생산큐_항목의_부족분은_주문수량에서_재고를_뺀_값이다)
    {
        SeedSample("S001", 10.0, 0.9, 3);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.ApproveOrder("ORD-1");

        const auto item = productionQueueRepository_.FindById("ORD-1");
        ASSERT_TRUE(item.has_value());
        // 부족분 = 10 - 3 = 7
        EXPECT_EQ(item->GetShortageQuantity(), 7);
    }

    TEST_F(OrderApprovalControllerTest, 부족분이_수율로_정확히_나누어떨어지면_실생산량은_그_몫과_같다)
    {
        // 부족분 8, 수율 0.8 -> 8 / 0.8 = 10.0 (나누어떨어짐) -> ceil(10.0) = 10
        SeedSample("S001", 10.0, 0.8, 2);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.ApproveOrder("ORD-1");

        const auto item = productionQueueRepository_.FindById("ORD-1");
        ASSERT_TRUE(item.has_value());
        EXPECT_EQ(item->GetActualProductionQuantity(), 10);
    }

    TEST_F(OrderApprovalControllerTest, 부족분이_수율로_나누어떨어지지_않으면_실생산량은_올림된값이다)
    {
        // 부족분 7, 수율 0.9 -> 7 / 0.9 = 7.777... -> ceil = 8
        SeedSample("S001", 10.0, 0.9, 3);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.ApproveOrder("ORD-1");

        const auto item = productionQueueRepository_.FindById("ORD-1");
        ASSERT_TRUE(item.has_value());
        EXPECT_EQ(item->GetActualProductionQuantity(), 8);
    }

    TEST_F(OrderApprovalControllerTest, 수율이_1이면_실생산량은_부족분과_같다)
    {
        // 부족분 7, 수율 1.0 -> ceil(7/1.0) = 7 (불량 없음)
        SeedSample("S001", 10.0, 1.0, 3);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.ApproveOrder("ORD-1");

        const auto item = productionQueueRepository_.FindById("ORD-1");
        ASSERT_TRUE(item.has_value());
        EXPECT_EQ(item->GetActualProductionQuantity(), 7);
    }

    TEST_F(OrderApprovalControllerTest, 수율이_매우_낮으면_실생산량이_부족분보다_크게_계산된다)
    {
        // 부족분 5, 수율 0.1 -> ceil(5/0.1) = 50
        SeedSample("S001", 10.0, 0.1, 5);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.ApproveOrder("ORD-1");

        const auto item = productionQueueRepository_.FindById("ORD-1");
        ASSERT_TRUE(item.has_value());
        EXPECT_EQ(item->GetActualProductionQuantity(), 50);
    }

    TEST_F(OrderApprovalControllerTest, 총생산시간은_평균생산시간과_실생산량의_곱이다)
    {
        // 부족분 7, 수율 0.9 -> 실생산량 8, 평균생산시간 10.0 -> 총생산시간 80.0
        SeedSample("S001", 10.0, 0.9, 3);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.ApproveOrder("ORD-1");

        const auto item = productionQueueRepository_.FindById("ORD-1");
        ASSERT_TRUE(item.has_value());
        EXPECT_DOUBLE_EQ(item->GetTotalProductionTime().count(), 80.0);
    }

    TEST_F(OrderApprovalControllerTest, 생산큐_항목의_주문번호는_승인한_주문의_주문번호와_같다)
    {
        SeedSample("S001", 10.0, 0.9, 3);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.ApproveOrder("ORD-1");

        const auto item = productionQueueRepository_.FindById("ORD-1");
        ASSERT_TRUE(item.has_value());
        EXPECT_EQ(item->GetOrderId(), "ORD-1");
    }

    // ---- ApproveOrder: 재고 물리적 미차감 ----

    TEST_F(OrderApprovalControllerTest, 승인_후에도_시료_재고는_승인_전과_동일하다_재고충분케이스)
    {
        SeedSample("S001", 10.0, 0.9, 20);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.ApproveOrder("ORD-1");

        const auto sample = sampleRepository_.FindById("S001");
        ASSERT_TRUE(sample.has_value());
        EXPECT_EQ(sample->GetStock(), 20);
    }

    TEST_F(OrderApprovalControllerTest, 승인_후에도_시료_재고는_승인_전과_동일하다_재고부족케이스)
    {
        SeedSample("S001", 10.0, 0.9, 3);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.ApproveOrder("ORD-1");

        const auto sample = sampleRepository_.FindById("S001");
        ASSERT_TRUE(sample.has_value());
        EXPECT_EQ(sample->GetStock(), 3);
    }

    // ---- ApproveOrder: 존재하지 않는 주문/시료 ----

    TEST_F(OrderApprovalControllerTest, 존재하지_않는_주문번호로_승인하면_OrderNotFound를_반환한다)
    {
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        const auto result = approvalController.ApproveOrder("NO-SUCH-ORDER");

        EXPECT_EQ(result, OrderApprovalResult::OrderNotFound);
    }

    TEST_F(OrderApprovalControllerTest, 존재하지_않는_주문번호로_승인해도_생산큐는_변경되지_않는다)
    {
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.ApproveOrder("NO-SUCH-ORDER");

        EXPECT_TRUE(productionQueueRepository_.GetAll().empty());
    }

    TEST_F(OrderApprovalControllerTest, 주문이_참조하는_시료가_존재하지_않으면_SampleNotFound를_반환한다)
    {
        SeedOrder("ORD-1", "MISSING-SAMPLE", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        const auto result = approvalController.ApproveOrder("ORD-1");

        EXPECT_EQ(result, OrderApprovalResult::SampleNotFound);
    }

    TEST_F(OrderApprovalControllerTest, 시료를_찾지_못해_SampleNotFound가_되면_주문상태는_RESERVED로_유지된다)
    {
        SeedOrder("ORD-1", "MISSING-SAMPLE", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.ApproveOrder("ORD-1");

        const auto updated = orderRepository_.FindById("ORD-1");
        ASSERT_TRUE(updated.has_value());
        EXPECT_EQ(updated->GetStatus(), OrderStatus::Reserved);
    }

    // ---- ApproveOrder: 이미 RESERVED가 아닌 주문 ----

    TEST_F(OrderApprovalControllerTest, 이미_REJECTED인_주문을_승인하면_InvalidOrderState를_반환한다)
    {
        SeedSample("S001", 10.0, 0.9, 20);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Rejected);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        const auto result = approvalController.ApproveOrder("ORD-1");

        EXPECT_EQ(result, OrderApprovalResult::InvalidOrderState);
    }

    TEST_F(OrderApprovalControllerTest, 이미_PRODUCING인_주문을_승인하면_InvalidOrderState를_반환한다)
    {
        SeedSample("S001", 10.0, 0.9, 20);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Producing);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        const auto result = approvalController.ApproveOrder("ORD-1");

        EXPECT_EQ(result, OrderApprovalResult::InvalidOrderState);
    }

    TEST_F(OrderApprovalControllerTest, 이미_CONFIRMED인_주문을_승인하면_InvalidOrderState를_반환한다)
    {
        SeedSample("S001", 10.0, 0.9, 20);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Confirmed);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        const auto result = approvalController.ApproveOrder("ORD-1");

        EXPECT_EQ(result, OrderApprovalResult::InvalidOrderState);
    }

    TEST_F(OrderApprovalControllerTest, 이미_RELEASED인_주문을_승인하면_InvalidOrderState를_반환한다)
    {
        SeedSample("S001", 10.0, 0.9, 20);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Released);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        const auto result = approvalController.ApproveOrder("ORD-1");

        EXPECT_EQ(result, OrderApprovalResult::InvalidOrderState);
    }

    TEST_F(OrderApprovalControllerTest, RESERVED가_아닌_주문에_대한_승인시도는_상태를_바꾸지_않는다)
    {
        SeedSample("S001", 10.0, 0.9, 20);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Rejected);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.ApproveOrder("ORD-1");

        const auto updated = orderRepository_.FindById("ORD-1");
        ASSERT_TRUE(updated.has_value());
        EXPECT_EQ(updated->GetStatus(), OrderStatus::Rejected);
    }

    // ---- ApproveOrder: 병합 없는 독립 생산 큐 항목 ----

    TEST_F(OrderApprovalControllerTest, 동일_시료를_참조하는_서로_다른_두_주문을_모두_승인하면_생산큐에_2건이_생성된다)
    {
        SeedSample("S001", 10.0, 0.9, 3);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        SeedOrder("ORD-2", "S001", "Customer-B", 8, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.ApproveOrder("ORD-1");
        approvalController.ApproveOrder("ORD-2");

        EXPECT_EQ(productionQueueRepository_.GetAll().size(), 2u);
    }

    TEST_F(OrderApprovalControllerTest, 동일_시료의_두_주문을_승인하면_각_생산큐_항목의_주문번호가_서로_다르다)
    {
        SeedSample("S001", 10.0, 0.9, 3);
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        SeedOrder("ORD-2", "S001", "Customer-B", 8, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.ApproveOrder("ORD-1");
        approvalController.ApproveOrder("ORD-2");

        const auto item1 = productionQueueRepository_.FindById("ORD-1");
        const auto item2 = productionQueueRepository_.FindById("ORD-2");
        ASSERT_TRUE(item1.has_value());
        ASSERT_TRUE(item2.has_value());
        // 재고 조회는 승인 시점마다 동일 저장소 값(3)을 기준으로 하므로(승인 후 물리적 차감 없음),
        // 각각 부족분이 독립적으로 계산된다: ORD-1 부족분 7, ORD-2 부족분 5.
        EXPECT_EQ(item1->GetShortageQuantity(), 7);
        EXPECT_EQ(item2->GetShortageQuantity(), 5);
    }

    // ---- RejectOrder: 정상 케이스 ----

    TEST_F(OrderApprovalControllerTest, RESERVED_주문을_거절하면_Success를_반환한다)
    {
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        const auto result = approvalController.RejectOrder("ORD-1");

        EXPECT_EQ(result, OrderRejectionResult::Success);
    }

    TEST_F(OrderApprovalControllerTest, RESERVED_주문을_거절하면_상태가_REJECTED로_전환된다)
    {
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Reserved);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.RejectOrder("ORD-1");

        const auto updated = orderRepository_.FindById("ORD-1");
        ASSERT_TRUE(updated.has_value());
        EXPECT_EQ(updated->GetStatus(), OrderStatus::Rejected);
    }

    // ---- RejectOrder: 이미 종단/다른 상태인 주문 ----

    TEST_F(OrderApprovalControllerTest, 이미_REJECTED인_주문을_다시_거절하면_InvalidOrderState를_반환한다)
    {
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Rejected);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        const auto result = approvalController.RejectOrder("ORD-1");

        EXPECT_EQ(result, OrderRejectionResult::InvalidOrderState);
    }

    TEST_F(OrderApprovalControllerTest, 이미_REJECTED인_주문을_다시_거절해도_상태는_REJECTED로_유지된다)
    {
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Rejected);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        approvalController.RejectOrder("ORD-1");

        const auto updated = orderRepository_.FindById("ORD-1");
        ASSERT_TRUE(updated.has_value());
        EXPECT_EQ(updated->GetStatus(), OrderStatus::Rejected);
    }

    TEST_F(OrderApprovalControllerTest, PRODUCING_상태의_주문을_거절하면_InvalidOrderState를_반환한다)
    {
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Producing);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        const auto result = approvalController.RejectOrder("ORD-1");

        EXPECT_EQ(result, OrderRejectionResult::InvalidOrderState);
    }

    TEST_F(OrderApprovalControllerTest, CONFIRMED_상태의_주문을_거절하면_InvalidOrderState를_반환한다)
    {
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Confirmed);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        const auto result = approvalController.RejectOrder("ORD-1");

        EXPECT_EQ(result, OrderRejectionResult::InvalidOrderState);
    }

    TEST_F(OrderApprovalControllerTest, RELEASED_상태의_주문을_거절하면_InvalidOrderState를_반환한다)
    {
        SeedOrder("ORD-1", "S001", "Customer-A", 10, OrderStatus::Released);
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        const auto result = approvalController.RejectOrder("ORD-1");

        EXPECT_EQ(result, OrderRejectionResult::InvalidOrderState);
    }

    // ---- RejectOrder: 존재하지 않는 주문 ----

    TEST_F(OrderApprovalControllerTest, 존재하지_않는_주문번호로_거절하면_OrderNotFound를_반환한다)
    {
        auto sampleController = MakeSampleController();
        OrderApprovalController approvalController(orderRepository_, sampleController, productionQueueRepository_);

        const auto result = approvalController.RejectOrder("NO-SUCH-ORDER");

        EXPECT_EQ(result, OrderRejectionResult::OrderNotFound);
    }
}
