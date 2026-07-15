#include <gtest/gtest.h>

#include <chrono>

#include "../src/Model/ProductionQueueItem.h"

using Model::ProductionQueueItem;

namespace
{
    ProductionQueueItem::TimePoint SampleStartTime()
    {
        return ProductionQueueItem::TimePoint(std::chrono::seconds(1'700'000'000));
    }

    TEST(ProductionQueueItem, 생성자로_전달한_주문ID를_GetOrderId가_그대로_반환한다)
    {
        ProductionQueueItem item(
            "O001", 5, 6, ProductionQueueItem::MinutesDuration(30.0), SampleStartTime());

        EXPECT_EQ(item.GetOrderId(), "O001");
    }

    TEST(ProductionQueueItem, 생성자로_전달한_부족분을_GetShortageQuantity가_그대로_반환한다)
    {
        ProductionQueueItem item(
            "O001", 5, 6, ProductionQueueItem::MinutesDuration(30.0), SampleStartTime());

        EXPECT_EQ(item.GetShortageQuantity(), 5);
    }

    TEST(ProductionQueueItem, 생성자로_전달한_실생산량을_GetActualProductionQuantity가_그대로_반환한다)
    {
        ProductionQueueItem item(
            "O001", 5, 6, ProductionQueueItem::MinutesDuration(30.0), SampleStartTime());

        EXPECT_EQ(item.GetActualProductionQuantity(), 6);
    }

    TEST(ProductionQueueItem, 생성자로_전달한_총생산시간을_GetTotalProductionTime이_그대로_반환한다)
    {
        ProductionQueueItem item(
            "O001", 5, 6, ProductionQueueItem::MinutesDuration(30.0), SampleStartTime());

        EXPECT_DOUBLE_EQ(item.GetTotalProductionTime().count(), 30.0);
    }

    TEST(ProductionQueueItem, 생성자로_전달한_생산시작시각을_GetProductionStartTime이_그대로_반환한다)
    {
        const ProductionQueueItem::TimePoint startTime = SampleStartTime();
        ProductionQueueItem item(
            "O001", 5, 6, ProductionQueueItem::MinutesDuration(30.0), startTime);

        EXPECT_EQ(item.GetProductionStartTime(), startTime);
    }

    // 예상 완료 시각 = 생산 시작 시각 + 총 생산 시간
    TEST(ProductionQueueItem, 총생산시간이_30분이면_예상완료시각은_시작시각으로부터_30분_뒤다)
    {
        const ProductionQueueItem::TimePoint startTime = SampleStartTime();
        ProductionQueueItem item(
            "O001", 5, 6, ProductionQueueItem::MinutesDuration(30.0), startTime);

        const auto expected = startTime + std::chrono::seconds(30 * 60);
        EXPECT_EQ(item.GetExpectedCompletionTime(), expected);
    }

    TEST(ProductionQueueItem, 총생산시간이_0분이면_예상완료시각은_시작시각과_동일하다)
    {
        const ProductionQueueItem::TimePoint startTime = SampleStartTime();
        ProductionQueueItem item(
            "O001", 0, 0, ProductionQueueItem::MinutesDuration(0.0), startTime);

        EXPECT_EQ(item.GetExpectedCompletionTime(), startTime);
    }

    TEST(ProductionQueueItem, 총생산시간이_소수점_분이면_예상완료시각에_소수점_초까지_반영된다)
    {
        const ProductionQueueItem::TimePoint startTime = SampleStartTime();
        // 12.5분 = 750초
        ProductionQueueItem item(
            "O001", 3, 4, ProductionQueueItem::MinutesDuration(12.5), startTime);

        const auto expected = startTime
            + std::chrono::duration_cast<ProductionQueueItem::TimePoint::duration>(
                std::chrono::duration<double, std::ratio<60>>(12.5));
        EXPECT_EQ(item.GetExpectedCompletionTime(), expected);
    }

    TEST(ProductionQueueItem, 부족분이_0이어도_생성된다)
    {
        ProductionQueueItem item(
            "O001", 0, 0, ProductionQueueItem::MinutesDuration(10.0), SampleStartTime());

        EXPECT_EQ(item.GetShortageQuantity(), 0);
    }
}
