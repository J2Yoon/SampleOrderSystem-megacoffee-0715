#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "../src/Controller/ProductionLineController.h"
#include "../src/Controller/SampleController.h"
#include "../src/Model/Order.h"
#include "../src/Model/OrderStatus.h"
#include "../src/Model/ProductionQueueItem.h"
#include "../src/Model/Sample.h"
#include "../src/Persistence/IOrderRepository.h"
#include "../src/Persistence/IProductionQueueRepository.h"
#include "../src/Persistence/ISampleRepository.h"

using Controller::ProductionLineController;
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

    // IProductionQueueRepository의 인메모리 페이크. push_back 순서를 그대로 유지해 FIFO 순서를 보장한다.
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

    class ProductionLineControllerTest : public ::testing::Test
    {
    protected:
        InMemorySampleRepository sampleRepository_;
        InMemoryOrderRepository orderRepository_;
        InMemoryProductionQueueRepository productionQueueRepository_;

        // 임의의 고정 기준 시각. 이 값 기준으로 상대 오프셋을 계산해 시간 정산 시나리오를 재현한다.
        const ProductionQueueItem::TimePoint baseTime_ = std::chrono::system_clock::now();

        SampleController MakeSampleController()
        {
            return SampleController(sampleRepository_);
        }

        ProductionLineController MakeController(SampleController& sampleController)
        {
            return ProductionLineController(productionQueueRepository_, orderRepository_, sampleController);
        }

        void SeedSample(const std::string& sampleId, int stock)
        {
            sampleRepository_.Create(Sample(sampleId, "Sample-" + sampleId, 10.0, 0.9, stock));
        }

        void SeedOrder(const std::string& orderId, const std::string& sampleId, int quantity, OrderStatus status)
        {
            orderRepository_.Create(Order(orderId, sampleId, "Customer", quantity, status, baseTime_));
        }

        void SeedQueueItem(const std::string& orderId, int shortageQuantity, int actualProductionQuantity,
            double totalProductionMinutes, ProductionQueueItem::TimePoint productionStartTime)
        {
            productionQueueRepository_.Create(ProductionQueueItem(
                orderId, shortageQuantity, actualProductionQuantity,
                ProductionQueueItem::MinutesDuration(totalProductionMinutes), productionStartTime));
        }

        static ProductionQueueItem::TimePoint AddMinutes(ProductionQueueItem::TimePoint timePoint, double minutes)
        {
            return timePoint + std::chrono::duration_cast<ProductionQueueItem::TimePoint::duration>(
                ProductionQueueItem::MinutesDuration(minutes));
        }
    };

    // ---- SettleCompletedItems: 빈 큐 ----

    TEST_F(ProductionLineControllerTest, 빈_큐에서_SettleCompletedItems는_0을_반환한다)
    {
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        EXPECT_EQ(controller.SettleCompletedItems(baseTime_), 0);
    }

    // ---- SettleCompletedItems: 단일 항목, 완료 전/후 ----

    TEST_F(ProductionLineControllerTest, 완료시각_이전이면_0건_정산된다)
    {
        SeedSample("S001", 3);
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Producing);
        SeedQueueItem("ORD-1", 7, 8, 60.0, baseTime_);
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        const auto settledCount = controller.SettleCompletedItems(AddMinutes(baseTime_, 30.0));

        EXPECT_EQ(settledCount, 0);
    }

    TEST_F(ProductionLineControllerTest, 완료시각_이전이면_주문_상태는_PRODUCING으로_유지된다)
    {
        SeedSample("S001", 3);
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Producing);
        SeedQueueItem("ORD-1", 7, 8, 60.0, baseTime_);
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        controller.SettleCompletedItems(AddMinutes(baseTime_, 30.0));

        const auto updatedOrder = orderRepository_.FindById("ORD-1");
        ASSERT_TRUE(updatedOrder.has_value());
        EXPECT_EQ(updatedOrder->GetStatus(), OrderStatus::Producing);
    }

    TEST_F(ProductionLineControllerTest, 완료시각_이전이면_재고는_변경되지_않는다)
    {
        SeedSample("S001", 3);
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Producing);
        SeedQueueItem("ORD-1", 7, 8, 60.0, baseTime_);
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        controller.SettleCompletedItems(AddMinutes(baseTime_, 30.0));

        const auto sample = sampleRepository_.FindById("S001");
        ASSERT_TRUE(sample.has_value());
        EXPECT_EQ(sample->GetStock(), 3);
    }

    TEST_F(ProductionLineControllerTest, 완료시각_이전이면_큐항목이_제거되지_않는다)
    {
        SeedSample("S001", 3);
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Producing);
        SeedQueueItem("ORD-1", 7, 8, 60.0, baseTime_);
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        controller.SettleCompletedItems(AddMinutes(baseTime_, 30.0));

        EXPECT_EQ(productionQueueRepository_.GetAll().size(), 1u);
    }

    TEST_F(ProductionLineControllerTest, 완료시각을_지나면_1건_정산된다)
    {
        SeedSample("S001", 3);
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Producing);
        SeedQueueItem("ORD-1", 7, 8, 60.0, baseTime_);
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        const auto settledCount = controller.SettleCompletedItems(AddMinutes(baseTime_, 61.0));

        EXPECT_EQ(settledCount, 1);
    }

    TEST_F(ProductionLineControllerTest, 완료시각을_지나면_재고가_실생산량만큼_증가한다)
    {
        SeedSample("S001", 3);
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Producing);
        SeedQueueItem("ORD-1", 7, 8, 60.0, baseTime_);
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        controller.SettleCompletedItems(AddMinutes(baseTime_, 61.0));

        const auto sample = sampleRepository_.FindById("S001");
        ASSERT_TRUE(sample.has_value());
        EXPECT_EQ(sample->GetStock(), 3 + 8);
    }

    TEST_F(ProductionLineControllerTest, 완료시각을_지나면_주문이_PRODUCING에서_CONFIRMED로_전환된다)
    {
        SeedSample("S001", 3);
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Producing);
        SeedQueueItem("ORD-1", 7, 8, 60.0, baseTime_);
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        controller.SettleCompletedItems(AddMinutes(baseTime_, 61.0));

        const auto updatedOrder = orderRepository_.FindById("ORD-1");
        ASSERT_TRUE(updatedOrder.has_value());
        EXPECT_EQ(updatedOrder->GetStatus(), OrderStatus::Confirmed);
    }

    TEST_F(ProductionLineControllerTest, 완료시각을_지나면_큐항목이_제거된다)
    {
        SeedSample("S001", 3);
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Producing);
        SeedQueueItem("ORD-1", 7, 8, 60.0, baseTime_);
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        controller.SettleCompletedItems(AddMinutes(baseTime_, 61.0));

        EXPECT_TRUE(productionQueueRepository_.GetAll().empty());
    }

    TEST_F(ProductionLineControllerTest, 완료시각과_정확히_같은_now에서는_정산된다)
    {
        // effectiveCompletionTime > now 인 경우에만 정산을 건너뛰므로, 정확히 같은 시각(now == completion)은
        // 정산 대상에 포함되어야 한다(완료 "즉시" 완료 상태여야 한다는 PRD 4.6.4의 취지).
        SeedSample("S001", 3);
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Producing);
        SeedQueueItem("ORD-1", 7, 8, 60.0, baseTime_);
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        const auto settledCount = controller.SettleCompletedItems(AddMinutes(baseTime_, 60.0));

        EXPECT_EQ(settledCount, 1);
    }

    // ---- SettleCompletedItems: FIFO 지연(cascading) ----

    TEST_F(ProductionLineControllerTest, 뒷항목의_자체_시작시각이_지났어도_앞항목이_아직_진행중이면_정산되지_않는다)
    {
        // ORD-1: baseTime_ 시작, 100분 소요 -> 실제 완료 baseTime_+100분.
        // ORD-2: baseTime_+10분(자체 승인 시각) 시작 예정, 5분 소요 -> 캐스케이딩 미적용시 자체 완료는
        // baseTime_+15분으로 이미 지났을 것처럼 보이지만, 단일 라인이라 ORD-1이 끝나야 시작 가능하다.
        SeedSample("S001", 3);
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Producing);
        SeedOrder("ORD-2", "S001", 5, OrderStatus::Producing);
        SeedQueueItem("ORD-1", 7, 8, 100.0, baseTime_);
        SeedQueueItem("ORD-2", 5, 5, 5.0, AddMinutes(baseTime_, 10.0));
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        const auto settledCount = controller.SettleCompletedItems(AddMinutes(baseTime_, 30.0));

        EXPECT_EQ(settledCount, 0);
    }

    TEST_F(ProductionLineControllerTest, FIFO_지연_상황에서는_두_큐항목_모두_그대로_남아있다)
    {
        SeedSample("S001", 3);
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Producing);
        SeedOrder("ORD-2", "S001", 5, OrderStatus::Producing);
        SeedQueueItem("ORD-1", 7, 8, 100.0, baseTime_);
        SeedQueueItem("ORD-2", 5, 5, 5.0, AddMinutes(baseTime_, 10.0));
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        controller.SettleCompletedItems(AddMinutes(baseTime_, 30.0));

        EXPECT_EQ(productionQueueRepository_.GetAll().size(), 2u);
    }

    TEST_F(ProductionLineControllerTest, 첫_항목이_완료된_뒤에는_두번째_항목의_실제_완료시각이_누적_계산된다)
    {
        // ORD-1은 baseTime_+100분에 완료. ORD-2는 그 이후에야 시작하므로 실제 완료는
        // baseTime_+100분+5분=baseTime_+105분. baseTime_+102분 시점에는 ORD-1만 정산되어야 한다.
        SeedSample("S001", 3);
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Producing);
        SeedOrder("ORD-2", "S001", 5, OrderStatus::Producing);
        SeedQueueItem("ORD-1", 7, 8, 100.0, baseTime_);
        SeedQueueItem("ORD-2", 5, 5, 5.0, AddMinutes(baseTime_, 10.0));
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        const auto settledCount = controller.SettleCompletedItems(AddMinutes(baseTime_, 102.0));

        EXPECT_EQ(settledCount, 1);
    }

    TEST_F(ProductionLineControllerTest, now를_아주_먼_미래로_주면_큐의_모든_항목이_순서대로_정산된다)
    {
        SeedSample("S001", 3);
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Producing);
        SeedOrder("ORD-2", "S001", 5, OrderStatus::Producing);
        SeedQueueItem("ORD-1", 7, 8, 100.0, baseTime_);
        SeedQueueItem("ORD-2", 5, 5, 5.0, AddMinutes(baseTime_, 10.0));
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        const auto settledCount = controller.SettleCompletedItems(AddMinutes(baseTime_, 100000.0));

        EXPECT_EQ(settledCount, 2);
    }

    TEST_F(ProductionLineControllerTest, now가_먼_미래이면_두_주문_모두_CONFIRMED로_전환되고_재고가_누적된다)
    {
        SeedSample("S001", 3);
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Producing);
        SeedOrder("ORD-2", "S001", 5, OrderStatus::Producing);
        SeedQueueItem("ORD-1", 7, 8, 100.0, baseTime_);
        SeedQueueItem("ORD-2", 5, 5, 5.0, AddMinutes(baseTime_, 10.0));
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        controller.SettleCompletedItems(AddMinutes(baseTime_, 100000.0));

        const auto order1 = orderRepository_.FindById("ORD-1");
        const auto order2 = orderRepository_.FindById("ORD-2");
        ASSERT_TRUE(order1.has_value());
        ASSERT_TRUE(order2.has_value());
        EXPECT_EQ(order1->GetStatus(), OrderStatus::Confirmed);
        EXPECT_EQ(order2->GetStatus(), OrderStatus::Confirmed);

        const auto sample = sampleRepository_.FindById("S001");
        ASSERT_TRUE(sample.has_value());
        EXPECT_EQ(sample->GetStock(), 3 + 8 + 5);
    }

    TEST_F(ProductionLineControllerTest, now가_먼_미래이면_큐가_비게_된다)
    {
        SeedSample("S001", 3);
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Producing);
        SeedOrder("ORD-2", "S001", 5, OrderStatus::Producing);
        SeedQueueItem("ORD-1", 7, 8, 100.0, baseTime_);
        SeedQueueItem("ORD-2", 5, 5, 5.0, AddMinutes(baseTime_, 10.0));
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        controller.SettleCompletedItems(AddMinutes(baseTime_, 100000.0));

        EXPECT_TRUE(productionQueueRepository_.GetAll().empty());
    }

    // ---- SettleCompletedItems: 주문을 찾을 수 없는 방어적 케이스 ----

    TEST_F(ProductionLineControllerTest, 큐항목이_가리키는_주문을_찾을수_없어도_예외없이_큐항목이_제거된다)
    {
        // 정상 흐름에서는 발생하지 않지만(설계 결정 3), 향후 주문 삭제 기능 대비 방어 코드 검증.
        SeedSample("S001", 3);
        SeedQueueItem("ORD-MISSING", 7, 8, 60.0, baseTime_);
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        EXPECT_NO_THROW(controller.SettleCompletedItems(AddMinutes(baseTime_, 61.0)));

        EXPECT_TRUE(productionQueueRepository_.GetAll().empty());
    }

    TEST_F(ProductionLineControllerTest, 큐항목이_가리키는_주문을_찾을수_없으면_재고는_변경되지_않는다)
    {
        SeedSample("S001", 3);
        SeedQueueItem("ORD-MISSING", 7, 8, 60.0, baseTime_);
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        controller.SettleCompletedItems(AddMinutes(baseTime_, 61.0));

        const auto sample = sampleRepository_.FindById("S001");
        ASSERT_TRUE(sample.has_value());
        EXPECT_EQ(sample->GetStock(), 3);
    }

    // ---- GetCurrentProductionStatus ----

    TEST_F(ProductionLineControllerTest, 큐가_비어있으면_GetCurrentProductionStatus는_nullopt를_반환한다)
    {
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        EXPECT_FALSE(controller.GetCurrentProductionStatus().has_value());
    }

    TEST_F(ProductionLineControllerTest, 큐에_항목이_있으면_GetCurrentProductionStatus는_맨앞_항목을_반환한다)
    {
        SeedSample("S001", 3);
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Producing);
        SeedOrder("ORD-2", "S001", 5, OrderStatus::Producing);
        SeedQueueItem("ORD-1", 7, 8, 100.0, baseTime_);
        SeedQueueItem("ORD-2", 5, 5, 5.0, AddMinutes(baseTime_, 10.0));
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        const auto status = controller.GetCurrentProductionStatus();

        ASSERT_TRUE(status.has_value());
        EXPECT_EQ(status->item.GetOrderId(), "ORD-1");
    }

    TEST_F(ProductionLineControllerTest, 첫_항목의_실제_시작시각은_승인시각과_같다)
    {
        // 첫 항목은 앞선 항목이 없으므로 캐스케이딩 지연이 없다.
        SeedSample("S001", 3);
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Producing);
        SeedQueueItem("ORD-1", 7, 8, 100.0, baseTime_);
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        const auto status = controller.GetCurrentProductionStatus();

        ASSERT_TRUE(status.has_value());
        EXPECT_EQ(status->effectiveStartTime, baseTime_);
    }

    // ---- GetWaitingQueue ----

    TEST_F(ProductionLineControllerTest, 큐가_비어있으면_GetWaitingQueue는_빈_벡터를_반환한다)
    {
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        EXPECT_TRUE(controller.GetWaitingQueue().empty());
    }

    TEST_F(ProductionLineControllerTest, 큐에_1건만_있으면_GetWaitingQueue는_비어있다)
    {
        SeedSample("S001", 3);
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Producing);
        SeedQueueItem("ORD-1", 7, 8, 100.0, baseTime_);
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        EXPECT_TRUE(controller.GetWaitingQueue().empty());
    }

    TEST_F(ProductionLineControllerTest, 큐에_여러건이_있으면_GetWaitingQueue는_맨앞을_제외한_나머지다)
    {
        SeedSample("S001", 3);
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Producing);
        SeedOrder("ORD-2", "S001", 5, OrderStatus::Producing);
        SeedOrder("ORD-3", "S001", 4, OrderStatus::Producing);
        SeedQueueItem("ORD-1", 7, 8, 100.0, baseTime_);
        SeedQueueItem("ORD-2", 5, 5, 5.0, AddMinutes(baseTime_, 10.0));
        SeedQueueItem("ORD-3", 4, 4, 3.0, AddMinutes(baseTime_, 20.0));
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        const auto waitingQueue = controller.GetWaitingQueue();

        ASSERT_EQ(waitingQueue.size(), 2u);
        EXPECT_EQ(waitingQueue[0].GetOrderId(), "ORD-2");
        EXPECT_EQ(waitingQueue[1].GetOrderId(), "ORD-3");
    }

    TEST_F(ProductionLineControllerTest, GetWaitingQueue는_FIFO_순서를_그대로_유지한다)
    {
        SeedSample("S001", 3);
        SeedOrder("ORD-1", "S001", 10, OrderStatus::Producing);
        SeedOrder("ORD-2", "S001", 5, OrderStatus::Producing);
        SeedOrder("ORD-3", "S001", 4, OrderStatus::Producing);
        SeedQueueItem("ORD-1", 7, 8, 100.0, baseTime_);
        SeedQueueItem("ORD-2", 5, 5, 5.0, AddMinutes(baseTime_, 10.0));
        SeedQueueItem("ORD-3", 4, 4, 3.0, AddMinutes(baseTime_, 20.0));
        auto sampleController = MakeSampleController();
        auto controller = MakeController(sampleController);

        const auto waitingQueue = controller.GetWaitingQueue();

        ASSERT_EQ(waitingQueue.size(), 2u);
        // 두 번째 항목(ORD-2)이 세 번째 항목(ORD-3)보다 먼저 나와야 FIFO 순서가 유지된 것이다.
        EXPECT_LT(std::distance(waitingQueue.begin(),
            std::find_if(waitingQueue.begin(), waitingQueue.end(),
                [](const ProductionQueueItem& item) { return item.GetOrderId() == "ORD-2"; })),
            std::distance(waitingQueue.begin(),
                std::find_if(waitingQueue.begin(), waitingQueue.end(),
                    [](const ProductionQueueItem& item) { return item.GetOrderId() == "ORD-3"; })));
    }
}
