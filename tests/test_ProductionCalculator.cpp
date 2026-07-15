#include <gtest/gtest.h>

#include "../src/Controller/ProductionCalculator.h"

using Controller::ProductionCalculator::CalculateActualProductionQuantity;
using Controller::ProductionCalculator::CalculateTotalProductionTime;

namespace
{
    // ---- CalculateActualProductionQuantity: ceil(부족분 / 수율) ----

    TEST(ProductionCalculatorTest, 부족분이_수율로_정확히_나누어떨어지면_그_몫과_같다)
    {
        // 8 / 0.8 = 10.0 (나누어떨어짐) -> ceil = 10
        EXPECT_EQ(CalculateActualProductionQuantity(8, 0.8), 10);
    }

    TEST(ProductionCalculatorTest, 부족분이_수율로_나누어떨어지지_않으면_올림된값이다)
    {
        // 7 / 0.9 = 7.777... -> ceil = 8
        EXPECT_EQ(CalculateActualProductionQuantity(7, 0.9), 8);
    }

    TEST(ProductionCalculatorTest, 수율이_1이면_실생산량은_부족분과_같다)
    {
        EXPECT_EQ(CalculateActualProductionQuantity(7, 1.0), 7);
    }

    TEST(ProductionCalculatorTest, 수율이_매우_낮으면_실생산량이_부족분보다_크게_계산된다)
    {
        // 5 / 0.1 = 50.0 -> ceil = 50
        EXPECT_EQ(CalculateActualProductionQuantity(5, 0.1), 50);
    }

    TEST(ProductionCalculatorTest, 부족분이_0이면_실생산량은_0이다)
    {
        EXPECT_EQ(CalculateActualProductionQuantity(0, 0.9), 0);
    }

    // ---- CalculateTotalProductionTime: 평균 생산시간 x 실 생산량 ----

    TEST(ProductionCalculatorTest, 총생산시간은_평균생산시간과_실생산량의_곱이다)
    {
        const auto totalProductionTime = CalculateTotalProductionTime(10.0, 8);
        EXPECT_DOUBLE_EQ(totalProductionTime.count(), 80.0);
    }

    TEST(ProductionCalculatorTest, 소수점_평균생산시간에_대한_총생산시간이_정확히_계산된다)
    {
        const auto totalProductionTime = CalculateTotalProductionTime(7.5, 4);
        EXPECT_DOUBLE_EQ(totalProductionTime.count(), 30.0);
    }

    TEST(ProductionCalculatorTest, 실생산량이_0이면_총생산시간도_0이다)
    {
        const auto totalProductionTime = CalculateTotalProductionTime(10.0, 0);
        EXPECT_DOUBLE_EQ(totalProductionTime.count(), 0.0);
    }
}
