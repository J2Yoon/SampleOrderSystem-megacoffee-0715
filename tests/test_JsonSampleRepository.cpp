#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "../src/Persistence/JsonSampleRepository.h"

using Model::Sample;
using Persistence::JsonSampleRepository;

namespace
{
    class JsonSampleRepositoryTest : public ::testing::Test
    {
    protected:
        std::filesystem::path testRootDirectory_;
        std::string filePath_;

        void SetUp() override
        {
            testRootDirectory_ = std::filesystem::temp_directory_path() / "SampleOrderSystemTests_SampleRepo";
            std::error_code errorCode;
            std::filesystem::remove_all(testRootDirectory_, errorCode);
            std::filesystem::create_directories(testRootDirectory_, errorCode);
            filePath_ = (testRootDirectory_ / "samples.json").string();
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

    Sample MakeSample(const std::string& id = "S001", int stock = 50)
    {
        return Sample(id, "Sample-A", 12.5, 0.9, stock);
    }

    // ---- 파일 없음 / 파싱 실패 폴백 ----

    TEST_F(JsonSampleRepositoryTest, 파일이_없으면_예외없이_빈_목록으로_시작한다)
    {
        JsonSampleRepository repository(filePath_);

        EXPECT_TRUE(repository.GetAll().empty());
    }

    TEST_F(JsonSampleRepositoryTest, 깨진_JSON_파일이면_예외없이_빈_목록으로_폴백한다)
    {
        WriteRawFile("{ this is not valid json ");

        JsonSampleRepository repository(filePath_);

        EXPECT_TRUE(repository.GetAll().empty());
    }

    TEST_F(JsonSampleRepositoryTest, 깨진_JSON_파일이어도_생성자가_예외를_던지지_않는다)
    {
        WriteRawFile("[{\"id\":");

        EXPECT_NO_THROW({ JsonSampleRepository repository(filePath_); });
    }

    // ---- Create ----

    TEST_F(JsonSampleRepositoryTest, Create하면_GetAll에_해당_시료가_포함된다)
    {
        JsonSampleRepository repository(filePath_);

        repository.Create(MakeSample("S001"));

        const auto all = repository.GetAll();
        ASSERT_EQ(all.size(), 1u);
        EXPECT_EQ(all[0].GetSampleId(), "S001");
    }

    TEST_F(JsonSampleRepositoryTest, Create는_성공하면_true를_반환한다)
    {
        JsonSampleRepository repository(filePath_);

        EXPECT_TRUE(repository.Create(MakeSample("S001")));
    }

    TEST_F(JsonSampleRepositoryTest, 중복된_시료ID로_Create하면_false를_반환한다)
    {
        JsonSampleRepository repository(filePath_);
        repository.Create(MakeSample("S001"));

        EXPECT_FALSE(repository.Create(MakeSample("S001")));
    }

    TEST_F(JsonSampleRepositoryTest, 중복된_시료ID로_Create를_시도해도_기존_데이터는_변하지_않는다)
    {
        JsonSampleRepository repository(filePath_);
        repository.Create(MakeSample("S001", 50));

        repository.Create(MakeSample("S001", 999));

        const auto found = repository.FindById("S001");
        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(found->GetStock(), 50);
    }

    // ---- FindById ----

    TEST_F(JsonSampleRepositoryTest, 등록된_시료ID로_FindById하면_값을_반환한다)
    {
        JsonSampleRepository repository(filePath_);
        repository.Create(MakeSample("S001"));

        EXPECT_TRUE(repository.FindById("S001").has_value());
    }

    TEST_F(JsonSampleRepositoryTest, 등록되지_않은_시료ID로_FindById하면_nullopt를_반환한다)
    {
        JsonSampleRepository repository(filePath_);

        EXPECT_FALSE(repository.FindById("UNKNOWN").has_value());
    }

    // ---- Update ----

    TEST_F(JsonSampleRepositoryTest, 존재하는_시료를_Update하면_true를_반환한다)
    {
        JsonSampleRepository repository(filePath_);
        repository.Create(MakeSample("S001", 50));

        EXPECT_TRUE(repository.Update(MakeSample("S001", 80)));
    }

    TEST_F(JsonSampleRepositoryTest, Update하면_변경된_값이_FindById에_반영된다)
    {
        JsonSampleRepository repository(filePath_);
        repository.Create(MakeSample("S001", 50));

        repository.Update(MakeSample("S001", 80));

        const auto found = repository.FindById("S001");
        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(found->GetStock(), 80);
    }

    TEST_F(JsonSampleRepositoryTest, 존재하지_않는_시료ID로_Update하면_false를_반환한다)
    {
        JsonSampleRepository repository(filePath_);

        EXPECT_FALSE(repository.Update(MakeSample("UNKNOWN")));
    }

    // ---- Remove ----

    TEST_F(JsonSampleRepositoryTest, 존재하는_시료를_Remove하면_true를_반환한다)
    {
        JsonSampleRepository repository(filePath_);
        repository.Create(MakeSample("S001"));

        EXPECT_TRUE(repository.Remove("S001"));
    }

    TEST_F(JsonSampleRepositoryTest, Remove하면_GetAll에서_해당_시료가_사라진다)
    {
        JsonSampleRepository repository(filePath_);
        repository.Create(MakeSample("S001"));

        repository.Remove("S001");

        EXPECT_TRUE(repository.GetAll().empty());
    }

    TEST_F(JsonSampleRepositoryTest, 존재하지_않는_시료ID로_Remove하면_false를_반환한다)
    {
        JsonSampleRepository repository(filePath_);

        EXPECT_FALSE(repository.Remove("UNKNOWN"));
    }

    // ---- 경계값 ----

    TEST_F(JsonSampleRepositoryTest, 재고가_0인_시료도_정상적으로_Create된다)
    {
        JsonSampleRepository repository(filePath_);

        repository.Create(MakeSample("S001", 0));

        const auto found = repository.FindById("S001");
        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(found->GetStock(), 0);
    }

    TEST_F(JsonSampleRepositoryTest, 수율이_1_0인_시료도_왕복시_정확히_보존된다)
    {
        JsonSampleRepository repository(filePath_);
        repository.Create(Sample("S001", "Sample-A", 12.5, 1.0, 50));

        JsonSampleRepository reloaded(filePath_);

        const auto found = reloaded.FindById("S001");
        ASSERT_TRUE(found.has_value());
        EXPECT_DOUBLE_EQ(found->GetYield(), 1.0);
    }

    // ---- 재시작(재적재) 시나리오: 가장 중요한 DoD ----

    TEST_F(JsonSampleRepositoryTest, 저장후_새_인스턴스로_재적재하면_시료개수가_동일하다)
    {
        {
            JsonSampleRepository repository(filePath_);
            repository.Create(MakeSample("S001", 50));
            repository.Create(MakeSample("S002", 30));
        }

        JsonSampleRepository reloaded(filePath_);

        EXPECT_EQ(reloaded.GetAll().size(), 2u);
    }

    TEST_F(JsonSampleRepositoryTest, 저장후_새_인스턴스로_재적재하면_모든_필드가_동일하다)
    {
        {
            JsonSampleRepository repository(filePath_);
            repository.Create(Sample("S001", "Sample-A", 12.5, 0.85, 50));
        }

        JsonSampleRepository reloaded(filePath_);
        const auto found = reloaded.FindById("S001");

        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(found->GetSampleId(), "S001");
        EXPECT_EQ(found->GetName(), "Sample-A");
        EXPECT_DOUBLE_EQ(found->GetAverageProductionMinutesPerUnit(), 12.5);
        EXPECT_DOUBLE_EQ(found->GetYield(), 0.85);
        EXPECT_EQ(found->GetStock(), 50);
    }

    TEST_F(JsonSampleRepositoryTest, 저장후_재적재해도_Remove로_삭제된_시료는_다시_나타나지_않는다)
    {
        {
            JsonSampleRepository repository(filePath_);
            repository.Create(MakeSample("S001"));
            repository.Create(MakeSample("S002"));
            repository.Remove("S001");
        }

        JsonSampleRepository reloaded(filePath_);

        EXPECT_FALSE(reloaded.FindById("S001").has_value());
        EXPECT_TRUE(reloaded.FindById("S002").has_value());
    }
}
