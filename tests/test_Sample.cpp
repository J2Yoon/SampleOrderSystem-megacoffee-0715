#include <gtest/gtest.h>

#include "../src/Model/Sample.h"

using Model::Sample;

namespace
{
    TEST(Sample, 생성자로_전달한_시료ID를_GetSampleId가_그대로_반환한다)
    {
        Sample sample("S001", "Wafer-A", 12.5, 0.9, 100);

        EXPECT_EQ(sample.GetSampleId(), "S001");
    }

    TEST(Sample, 생성자로_전달한_이름을_GetName이_그대로_반환한다)
    {
        Sample sample("S001", "Wafer-A", 12.5, 0.9, 100);

        EXPECT_EQ(sample.GetName(), "Wafer-A");
    }

    TEST(Sample, 생성자로_전달한_평균생산시간을_GetAverageProductionMinutesPerUnit이_그대로_반환한다)
    {
        Sample sample("S001", "Wafer-A", 12.5, 0.9, 100);

        EXPECT_DOUBLE_EQ(sample.GetAverageProductionMinutesPerUnit(), 12.5);
    }

    TEST(Sample, 생성자로_전달한_수율을_GetYield가_그대로_반환한다)
    {
        Sample sample("S001", "Wafer-A", 12.5, 0.9, 100);

        EXPECT_DOUBLE_EQ(sample.GetYield(), 0.9);
    }

    TEST(Sample, 생성자로_전달한_재고를_GetStock이_그대로_반환한다)
    {
        Sample sample("S001", "Wafer-A", 12.5, 0.9, 100);

        EXPECT_EQ(sample.GetStock(), 100);
    }

    TEST(Sample, 수율이_1_0인_시료도_정상적으로_생성된다)
    {
        Sample sample("S002", "Wafer-B", 5.0, 1.0, 50);

        EXPECT_DOUBLE_EQ(sample.GetYield(), 1.0);
    }

    TEST(Sample, 재고가_0인_시료도_정상적으로_생성된다)
    {
        Sample sample("S003", "Wafer-C", 5.0, 0.5, 0);

        EXPECT_EQ(sample.GetStock(), 0);
    }

    TEST(Sample, SetStock_호출하면_재고값이_새_값으로_갱신된다)
    {
        Sample sample("S001", "Wafer-A", 12.5, 0.9, 100);

        sample.SetStock(30);

        EXPECT_EQ(sample.GetStock(), 30);
    }

    TEST(Sample, SetStock으로_재고를_0으로_갱신할_수_있다)
    {
        Sample sample("S001", "Wafer-A", 12.5, 0.9, 100);

        sample.SetStock(0);

        EXPECT_EQ(sample.GetStock(), 0);
    }
}
