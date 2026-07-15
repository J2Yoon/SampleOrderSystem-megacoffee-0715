#include <gtest/gtest.h>

#include <chrono>

#include "../src/Model/Order.h"
#include "../src/Model/OrderStatus.h"

using Model::Order;
using Model::OrderStatus;

namespace
{
    Order::TimePoint SampleCreatedAt()
    {
        return std::chrono::system_clock::time_point(std::chrono::seconds(1'700'000'000));
    }

    // 신규 접수용 생성자 (상태는 항상 RESERVED로 고정 — docs/PRD.md 4.3.1)
    TEST(OrderNewConstructor, 신규_접수용_생성자로_생성하면_상태가_Reserved로_고정된다)
    {
        Order order("O001", "S001", "Customer-A", 10, SampleCreatedAt());

        EXPECT_EQ(order.GetStatus(), OrderStatus::Reserved);
    }

    TEST(OrderNewConstructor, 신규_접수용_생성자의_주문ID를_GetOrderId가_그대로_반환한다)
    {
        Order order("O001", "S001", "Customer-A", 10, SampleCreatedAt());

        EXPECT_EQ(order.GetOrderId(), "O001");
    }

    TEST(OrderNewConstructor, 신규_접수용_생성자의_시료ID를_GetSampleId가_그대로_반환한다)
    {
        Order order("O001", "S001", "Customer-A", 10, SampleCreatedAt());

        EXPECT_EQ(order.GetSampleId(), "S001");
    }

    TEST(OrderNewConstructor, 신규_접수용_생성자의_고객명을_GetCustomerName이_그대로_반환한다)
    {
        Order order("O001", "S001", "Customer-A", 10, SampleCreatedAt());

        EXPECT_EQ(order.GetCustomerName(), "Customer-A");
    }

    TEST(OrderNewConstructor, 신규_접수용_생성자의_수량을_GetQuantity가_그대로_반환한다)
    {
        Order order("O001", "S001", "Customer-A", 10, SampleCreatedAt());

        EXPECT_EQ(order.GetQuantity(), 10);
    }

    TEST(OrderNewConstructor, 신규_접수용_생성자의_생성일시를_GetCreatedAt이_그대로_반환한다)
    {
        const Order::TimePoint createdAt = SampleCreatedAt();
        Order order("O001", "S001", "Customer-A", 10, createdAt);

        EXPECT_EQ(order.GetCreatedAt(), createdAt);
    }

    TEST(OrderNewConstructor, 주문수량이_0이어도_생성된다)
    {
        Order order("O001", "S001", "Customer-A", 0, SampleCreatedAt());

        EXPECT_EQ(order.GetQuantity(), 0);
    }

    // 영속성 복원용 생성자 (임의 상태 지정 가능)
    TEST(OrderRestoreConstructor, 영속성_복원용_생성자로_Confirmed_상태를_그대로_지정할_수_있다)
    {
        Order order("O002", "S001", "Customer-B", 5, OrderStatus::Confirmed, SampleCreatedAt());

        EXPECT_EQ(order.GetStatus(), OrderStatus::Confirmed);
    }

    TEST(OrderRestoreConstructor, 영속성_복원용_생성자로_Rejected_상태를_그대로_지정할_수_있다)
    {
        Order order("O003", "S001", "Customer-C", 5, OrderStatus::Rejected, SampleCreatedAt());

        EXPECT_EQ(order.GetStatus(), OrderStatus::Rejected);
    }

    TEST(OrderRestoreConstructor, 영속성_복원용_생성자의_주문ID를_GetOrderId가_그대로_반환한다)
    {
        Order order("O002", "S001", "Customer-B", 5, OrderStatus::Confirmed, SampleCreatedAt());

        EXPECT_EQ(order.GetOrderId(), "O002");
    }

    // TryTransitionTo — 유효한 전이
    TEST(OrderTryTransitionTo, Reserved에서_Confirmed로_전이하면_true를_반환한다)
    {
        Order order("O001", "S001", "Customer-A", 10, SampleCreatedAt());

        EXPECT_TRUE(order.TryTransitionTo(OrderStatus::Confirmed));
    }

    TEST(OrderTryTransitionTo, Reserved에서_Confirmed로_전이하면_실제_상태가_Confirmed로_바뀐다)
    {
        Order order("O001", "S001", "Customer-A", 10, SampleCreatedAt());

        order.TryTransitionTo(OrderStatus::Confirmed);

        EXPECT_EQ(order.GetStatus(), OrderStatus::Confirmed);
    }

    TEST(OrderTryTransitionTo, Reserved에서_Rejected로_전이하면_true를_반환한다)
    {
        Order order("O001", "S001", "Customer-A", 10, SampleCreatedAt());

        EXPECT_TRUE(order.TryTransitionTo(OrderStatus::Rejected));
    }

    TEST(OrderTryTransitionTo, Producing에서_Confirmed로_전이하면_true를_반환한다)
    {
        Order order("O001", "S001", "Customer-A", 10, OrderStatus::Producing, SampleCreatedAt());

        EXPECT_TRUE(order.TryTransitionTo(OrderStatus::Confirmed));
    }

    TEST(OrderTryTransitionTo, Confirmed에서_Released로_전이하면_true를_반환한다)
    {
        Order order("O001", "S001", "Customer-A", 10, OrderStatus::Confirmed, SampleCreatedAt());

        EXPECT_TRUE(order.TryTransitionTo(OrderStatus::Released));
    }

    // TryTransitionTo — 유효하지 않은 전이 (상태 전이 위반)
    TEST(OrderTryTransitionTo, Rejected상태에서_Confirmed로_재전이_시도하면_false를_반환한다)
    {
        Order order("O001", "S001", "Customer-A", 10, OrderStatus::Rejected, SampleCreatedAt());

        EXPECT_FALSE(order.TryTransitionTo(OrderStatus::Confirmed));
    }

    TEST(OrderTryTransitionTo, Rejected상태에서_Confirmed로_재전이_시도해도_상태는_Rejected로_유지된다)
    {
        Order order("O001", "S001", "Customer-A", 10, OrderStatus::Rejected, SampleCreatedAt());

        order.TryTransitionTo(OrderStatus::Confirmed);

        EXPECT_EQ(order.GetStatus(), OrderStatus::Rejected);
    }

    TEST(OrderTryTransitionTo, Released상태에서_다시_출고_시도하면_false를_반환한다)
    {
        Order order("O001", "S001", "Customer-A", 10, OrderStatus::Released, SampleCreatedAt());

        EXPECT_FALSE(order.TryTransitionTo(OrderStatus::Released));
    }

    TEST(OrderTryTransitionTo, Released상태에서_다시_출고_시도해도_상태는_Released로_유지된다)
    {
        Order order("O001", "S001", "Customer-A", 10, OrderStatus::Released, SampleCreatedAt());

        order.TryTransitionTo(OrderStatus::Released);

        EXPECT_EQ(order.GetStatus(), OrderStatus::Released);
    }

    TEST(OrderTryTransitionTo, Reserved상태에서_Released로_직접_전이_시도하면_false를_반환한다)
    {
        Order order("O001", "S001", "Customer-A", 10, SampleCreatedAt());

        EXPECT_FALSE(order.TryTransitionTo(OrderStatus::Released));
    }
}
