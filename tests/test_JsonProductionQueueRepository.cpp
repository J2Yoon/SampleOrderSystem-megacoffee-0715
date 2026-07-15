#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>

#include "../src/Persistence/JsonProductionQueueRepository.h"

using Model::ProductionQueueItem;
using Persistence::JsonProductionQueueRepository;

namespace
{
    class JsonProductionQueueRepositoryTest : public ::testing::Test
    {
    protected:
        std::filesystem::path testRootDirectory_;
        std::string filePath_;

        void SetUp() override
        {
            testRootDirectory_ = std::filesystem::temp_directory_path() / "SampleOrderSystemTests_ProductionQueueRepo";
            std::error_code errorCode;
            std::filesystem::remove_all(testRootDirectory_, errorCode);
            std::filesystem::create_directories(testRootDirectory_, errorCode);
            filePath_ = (testRootDirectory_ / "productionQueue.json").string();
        }

        void TearDown() override
        {
            std::error_code errorCode;
            std::filesystem::remove_all(testRootDirectory_, errorCode);
        }

        void WriteRawFile(const std::string& content) const
        {
            std::ofstream file(filePath_, std::ios::binary | std::ios::trunc);
            file << content;
        }
    };

    ProductionQueueItem::TimePoint SampleStartTime()
    {
        return ProductionQueueItem::TimePoint(std::chrono::milliseconds(1'700'000'000'123LL));
    }

    ProductionQueueItem MakeItem(const std::string& orderId = "O001")
    {
        return ProductionQueueItem(
            orderId,
            /*shortageQuantity=*/5,
            /*actualProductionQuantity=*/6,
            ProductionQueueItem::MinutesDuration(37.5),
            SampleStartTime());
    }

    // ---- 파일 없음 / 파싱 실패 폴백 ----

    TEST_F(JsonProductionQueueRepositoryTest, 파일이_없으면_예외없이_빈_목록으로_시작한다)
    {
        JsonProductionQueueRepository repository(filePath_);

        EXPECT_TRUE(repository.GetAll().empty());
    }

    TEST_F(JsonProductionQueueRepositoryTest, 깨진_JSON_파일이면_예외없이_빈_목록으로_폴백한다)
    {
        WriteRawFile("[{\"orderId\":");

        JsonProductionQueueRepository repository(filePath_);

        EXPECT_TRUE(repository.GetAll().empty());
    }

    TEST_F(JsonProductionQueueRepositoryTest, 깨진_JSON_파일이어도_생성자가_예외를_던지지_않는다)
    {
        WriteRawFile("{{{{");

        EXPECT_NO_THROW({ JsonProductionQueueRepository repository(filePath_); });
    }

    // ---- Create ----

    TEST_F(JsonProductionQueueRepositoryTest, Create하면_GetAll에_해당_항목이_포함된다)
    {
        JsonProductionQueueRepository repository(filePath_);

        repository.Create(MakeItem("O001"));

        const auto all = repository.GetAll();
        ASSERT_EQ(all.size(), 1u);
        EXPECT_EQ(all[0].GetOrderId(), "O001");
    }

    TEST_F(JsonProductionQueueRepositoryTest, 동일_주문번호로_Create하면_false를_반환한다_병합금지)
    {
        JsonProductionQueueRepository repository(filePath_);
        repository.Create(MakeItem("O001"));

        EXPECT_FALSE(repository.Create(MakeItem("O001")));
    }

    // ---- FindById ----

    TEST_F(JsonProductionQueueRepositoryTest, 등록된_주문번호로_FindById하면_값을_반환한다)
    {
        JsonProductionQueueRepository repository(filePath_);
        repository.Create(MakeItem("O001"));

        EXPECT_TRUE(repository.FindById("O001").has_value());
    }

    TEST_F(JsonProductionQueueRepositoryTest, 등록되지_않은_주문번호로_FindById하면_nullopt를_반환한다)
    {
        JsonProductionQueueRepository repository(filePath_);

        EXPECT_FALSE(repository.FindById("UNKNOWN").has_value());
    }

    // ---- Update ----

    TEST_F(JsonProductionQueueRepositoryTest, 존재하는_항목을_Update하면_true를_반환한다)
    {
        JsonProductionQueueRepository repository(filePath_);
        repository.Create(MakeItem("O001"));

        EXPECT_TRUE(repository.Update(MakeItem("O001")));
    }

    TEST_F(JsonProductionQueueRepositoryTest, 존재하지_않는_주문번호로_Update하면_false를_반환한다)
    {
        JsonProductionQueueRepository repository(filePath_);

        EXPECT_FALSE(repository.Update(MakeItem("UNKNOWN")));
    }

    // ---- Remove (생산 완료 후 큐에서 제거) ----

    TEST_F(JsonProductionQueueRepositoryTest, 존재하는_항목을_Remove하면_true를_반환한다)
    {
        JsonProductionQueueRepository repository(filePath_);
        repository.Create(MakeItem("O001"));

        EXPECT_TRUE(repository.Remove("O001"));
    }

    TEST_F(JsonProductionQueueRepositoryTest, Remove하면_GetAll에서_해당_항목이_사라진다)
    {
        JsonProductionQueueRepository repository(filePath_);
        repository.Create(MakeItem("O001"));

        repository.Remove("O001");

        EXPECT_TRUE(repository.GetAll().empty());
    }

    TEST_F(JsonProductionQueueRepositoryTest, 존재하지_않는_주문번호로_Remove하면_false를_반환한다)
    {
        JsonProductionQueueRepository repository(filePath_);

        EXPECT_FALSE(repository.Remove("UNKNOWN"));
    }

    // ---- 재시작(재적재) 시나리오: 가장 중요한 DoD ----

    TEST_F(JsonProductionQueueRepositoryTest, 저장후_새_인스턴스로_재적재하면_항목개수가_동일하다)
    {
        {
            JsonProductionQueueRepository repository(filePath_);
            repository.Create(MakeItem("O001"));
            repository.Create(MakeItem("O002"));
        }

        JsonProductionQueueRepository reloaded(filePath_);

        EXPECT_EQ(reloaded.GetAll().size(), 2u);
    }

    TEST_F(JsonProductionQueueRepositoryTest, 저장후_재적재하면_부족분과_실생산량이_동일하다)
    {
        {
            JsonProductionQueueRepository repository(filePath_);
            repository.Create(MakeItem("O001"));
        }

        JsonProductionQueueRepository reloaded(filePath_);
        const auto found = reloaded.FindById("O001");

        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(found->GetShortageQuantity(), 5);
        EXPECT_EQ(found->GetActualProductionQuantity(), 6);
    }

    TEST_F(JsonProductionQueueRepositoryTest, 저장후_재적재하면_생산시작시각이_밀리초_단위까지_정확히_보존된다)
    {
        const ProductionQueueItem::TimePoint startTime = SampleStartTime();
        {
            JsonProductionQueueRepository repository(filePath_);
            repository.Create(MakeItem("O001"));
        }

        JsonProductionQueueRepository reloaded(filePath_);
        const auto found = reloaded.FindById("O001");

        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(found->GetProductionStartTime(), startTime);
    }

    TEST_F(JsonProductionQueueRepositoryTest, 저장후_재적재하면_총생산시간이_정확히_보존된다)
    {
        {
            JsonProductionQueueRepository repository(filePath_);
            repository.Create(MakeItem("O001"));
        }

        JsonProductionQueueRepository reloaded(filePath_);
        const auto found = reloaded.FindById("O001");

        ASSERT_TRUE(found.has_value());
        EXPECT_DOUBLE_EQ(found->GetTotalProductionTime().count(), 37.5);
    }

    TEST_F(JsonProductionQueueRepositoryTest, 저장후_재적재하면_예상완료시각이_시작시각과_총생산시간으로부터_정확히_재계산된다)
    {
        const ProductionQueueItem::TimePoint startTime = SampleStartTime();
        const auto totalProductionTime = ProductionQueueItem::MinutesDuration(37.5);
        const ProductionQueueItem::TimePoint expectedCompletionTime =
            startTime + std::chrono::duration_cast<ProductionQueueItem::TimePoint::duration>(totalProductionTime);
        {
            JsonProductionQueueRepository repository(filePath_);
            repository.Create(MakeItem("O001"));
        }

        JsonProductionQueueRepository reloaded(filePath_);
        const auto found = reloaded.FindById("O001");

        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(found->GetExpectedCompletionTime(), expectedCompletionTime);
    }

    // ---- expectedCompletionEpochMillis는 파생값이며 파일 값과 무관하게 항상 재계산됨 ----

    TEST_F(JsonProductionQueueRepositoryTest, 파일에_틀린_expectedCompletionEpochMillis가_있어도_시작시각과_총생산시간으로_재계산된다)
    {
        // productionStartEpochMillis=1'700'000'000'123, totalProductionMinutes=37.5분
        // 정상적인 완료시각은 1'700'000'000'123 + 37.5*60*1000 = 1'700'000'002'373 이지만,
        // 파일에는 일부러 완전히 다른(터무니없는) 값을 넣어둔다.
        WriteRawFile(
            "[\n"
            "  {\n"
            "    \"orderId\": \"O001\",\n"
            "    \"shortageQuantity\": 5,\n"
            "    \"actualProductionQuantity\": 6,\n"
            "    \"totalProductionMinutes\": 37.5,\n"
            "    \"productionStartEpochMillis\": 1700000000123,\n"
            "    \"expectedCompletionEpochMillis\": 9999999999999\n"
            "  }\n"
            "]");

        JsonProductionQueueRepository repository(filePath_);
        const auto found = repository.FindById("O001");

        ASSERT_TRUE(found.has_value());
        const ProductionQueueItem::TimePoint expectedCompletionTime =
            SampleStartTime() + std::chrono::duration_cast<ProductionQueueItem::TimePoint::duration>(
                ProductionQueueItem::MinutesDuration(37.5));
        EXPECT_EQ(found->GetExpectedCompletionTime(), expectedCompletionTime);
    }

    // ---- FIFO 순서 보존 ----

    TEST_F(JsonProductionQueueRepositoryTest, 여러_생산항목을_생성한_순서가_저장후_재적재해도_그대로_유지된다)
    {
        {
            JsonProductionQueueRepository repository(filePath_);
            repository.Create(MakeItem("O001"));
            repository.Create(MakeItem("O002"));
            repository.Create(MakeItem("O003"));
        }

        JsonProductionQueueRepository reloaded(filePath_);
        const auto all = reloaded.GetAll();

        ASSERT_EQ(all.size(), 3u);
        EXPECT_EQ(all[0].GetOrderId(), "O001");
        EXPECT_EQ(all[1].GetOrderId(), "O002");
        EXPECT_EQ(all[2].GetOrderId(), "O003");
    }
}
