#include <gtest/gtest.h>

#include <filesystem>

#include "../src/Json/JsonIO.h"

using Json::FileIO;

namespace
{
    class JsonFileIOTest : public ::testing::Test
    {
    protected:
        std::filesystem::path testRootDirectory_;

        void SetUp() override
        {
            testRootDirectory_ = std::filesystem::temp_directory_path() / "SampleOrderSystemTests_JsonIO";
            std::error_code errorCode;
            std::filesystem::remove_all(testRootDirectory_, errorCode);
        }

        void TearDown() override
        {
            std::error_code errorCode;
            std::filesystem::remove_all(testRootDirectory_, errorCode);
        }

        std::string PathUnder(const std::string& relative) const
        {
            return (testRootDirectory_ / relative).string();
        }
    };

    TEST_F(JsonFileIOTest, 존재하지_않는_파일을_읽으면_nullopt를_반환한다)
    {
        const auto result = FileIO::ReadAllText(PathUnder("does_not_exist.json"));

        EXPECT_FALSE(result.has_value());
    }

    TEST_F(JsonFileIOTest, 쓰기_후_같은_경로를_읽으면_동일한_내용을_반환한다)
    {
        const std::string filePath = PathUnder("roundtrip.json");
        const std::string content = "{\"id\":\"S001\"}";

        ASSERT_TRUE(FileIO::WriteAllText(filePath, content));

        const auto result = FileIO::ReadAllText(filePath);

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, content);
    }

    TEST_F(JsonFileIOTest, WriteAllText는_성공하면_true를_반환한다)
    {
        EXPECT_TRUE(FileIO::WriteAllText(PathUnder("flag.json"), "{}"));
    }

    TEST_F(JsonFileIOTest, 상위_디렉터리가_없어도_쓰기_시_자동으로_생성된다)
    {
        const std::string filePath = PathUnder("nested/sub/dir/data.json");

        ASSERT_TRUE(FileIO::WriteAllText(filePath, "{}"));

        EXPECT_TRUE(std::filesystem::exists(filePath));
    }

    TEST_F(JsonFileIOTest, 기존_파일에_다시_쓰면_이전_내용을_덮어쓴다)
    {
        const std::string filePath = PathUnder("overwrite.json");
        FileIO::WriteAllText(filePath, "{\"value\":1}");

        FileIO::WriteAllText(filePath, "{\"value\":2}");

        const auto result = FileIO::ReadAllText(filePath);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, "{\"value\":2}");
    }
}
