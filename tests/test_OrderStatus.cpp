#include <gtest/gtest.h>

#include "../src/Model/OrderStatus.h"

using Model::OrderStatus;
using Model::IsValidOrderStatusTransition;

namespace
{
    // 허용된 전이 (docs/PRD.md 3절)
    TEST(OrderStatusTransition, Reserved에서_Rejected로_전이하면_허용된다)
    {
        EXPECT_TRUE(IsValidOrderStatusTransition(OrderStatus::Reserved, OrderStatus::Rejected));
    }

    TEST(OrderStatusTransition, Reserved에서_Confirmed로_전이하면_허용된다)
    {
        EXPECT_TRUE(IsValidOrderStatusTransition(OrderStatus::Reserved, OrderStatus::Confirmed));
    }

    TEST(OrderStatusTransition, Reserved에서_Producing으로_전이하면_허용된다)
    {
        EXPECT_TRUE(IsValidOrderStatusTransition(OrderStatus::Reserved, OrderStatus::Producing));
    }

    TEST(OrderStatusTransition, Producing에서_Confirmed로_전이하면_허용된다)
    {
        EXPECT_TRUE(IsValidOrderStatusTransition(OrderStatus::Producing, OrderStatus::Confirmed));
    }

    TEST(OrderStatusTransition, Confirmed에서_Released로_전이하면_허용된다)
    {
        EXPECT_TRUE(IsValidOrderStatusTransition(OrderStatus::Confirmed, OrderStatus::Released));
    }

    // Rejected(종단 상태)에서의 모든 전이는 차단
    TEST(OrderStatusTransition, Rejected에서_Reserved로_전이하면_차단된다)
    {
        EXPECT_FALSE(IsValidOrderStatusTransition(OrderStatus::Rejected, OrderStatus::Reserved));
    }

    TEST(OrderStatusTransition, Rejected에서_Confirmed로_전이하면_차단된다)
    {
        EXPECT_FALSE(IsValidOrderStatusTransition(OrderStatus::Rejected, OrderStatus::Confirmed));
    }

    TEST(OrderStatusTransition, Rejected에서_Producing으로_전이하면_차단된다)
    {
        EXPECT_FALSE(IsValidOrderStatusTransition(OrderStatus::Rejected, OrderStatus::Producing));
    }

    TEST(OrderStatusTransition, Rejected에서_Released로_전이하면_차단된다)
    {
        EXPECT_FALSE(IsValidOrderStatusTransition(OrderStatus::Rejected, OrderStatus::Released));
    }

    TEST(OrderStatusTransition, Rejected에서_Rejected로_자기자신_전이는_차단된다)
    {
        EXPECT_FALSE(IsValidOrderStatusTransition(OrderStatus::Rejected, OrderStatus::Rejected));
    }

    // Released(종단 상태)에서의 모든 전이는 차단
    TEST(OrderStatusTransition, Released에서_Reserved로_전이하면_차단된다)
    {
        EXPECT_FALSE(IsValidOrderStatusTransition(OrderStatus::Released, OrderStatus::Reserved));
    }

    TEST(OrderStatusTransition, Released에서_Rejected로_전이하면_차단된다)
    {
        EXPECT_FALSE(IsValidOrderStatusTransition(OrderStatus::Released, OrderStatus::Rejected));
    }

    TEST(OrderStatusTransition, Released에서_Producing으로_전이하면_차단된다)
    {
        EXPECT_FALSE(IsValidOrderStatusTransition(OrderStatus::Released, OrderStatus::Producing));
    }

    TEST(OrderStatusTransition, Released에서_Confirmed로_전이하면_차단된다)
    {
        EXPECT_FALSE(IsValidOrderStatusTransition(OrderStatus::Released, OrderStatus::Confirmed));
    }

    TEST(OrderStatusTransition, Released에서_Released로_자기자신_전이는_차단된다)
    {
        EXPECT_FALSE(IsValidOrderStatusTransition(OrderStatus::Released, OrderStatus::Released));
    }

    // 자기 자신으로의 전이 (그 외 상태)
    TEST(OrderStatusTransition, Reserved에서_Reserved로_자기자신_전이는_차단된다)
    {
        EXPECT_FALSE(IsValidOrderStatusTransition(OrderStatus::Reserved, OrderStatus::Reserved));
    }

    TEST(OrderStatusTransition, Producing에서_Producing으로_자기자신_전이는_차단된다)
    {
        EXPECT_FALSE(IsValidOrderStatusTransition(OrderStatus::Producing, OrderStatus::Producing));
    }

    TEST(OrderStatusTransition, Confirmed에서_Confirmed로_자기자신_전이는_차단된다)
    {
        EXPECT_FALSE(IsValidOrderStatusTransition(OrderStatus::Confirmed, OrderStatus::Confirmed));
    }

    // 역방향 전이
    TEST(OrderStatusTransition, Confirmed에서_Reserved로_역방향_전이는_차단된다)
    {
        EXPECT_FALSE(IsValidOrderStatusTransition(OrderStatus::Confirmed, OrderStatus::Reserved));
    }

    TEST(OrderStatusTransition, Confirmed에서_Producing으로_역방향_전이는_차단된다)
    {
        EXPECT_FALSE(IsValidOrderStatusTransition(OrderStatus::Confirmed, OrderStatus::Producing));
    }

    TEST(OrderStatusTransition, Producing에서_Reserved로_역방향_전이는_차단된다)
    {
        EXPECT_FALSE(IsValidOrderStatusTransition(OrderStatus::Producing, OrderStatus::Reserved));
    }

    // 그 외 정의되지 않은 조합
    TEST(OrderStatusTransition, Producing에서_Rejected로_전이하면_차단된다)
    {
        EXPECT_FALSE(IsValidOrderStatusTransition(OrderStatus::Producing, OrderStatus::Rejected));
    }

    TEST(OrderStatusTransition, Confirmed에서_Rejected로_전이하면_차단된다)
    {
        EXPECT_FALSE(IsValidOrderStatusTransition(OrderStatus::Confirmed, OrderStatus::Rejected));
    }
}
