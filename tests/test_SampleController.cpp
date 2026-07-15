#include <gtest/gtest.h>

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "../src/Controller/SampleController.h"
#include "../src/Persistence/ISampleRepository.h"

using Controller::SampleController;
using Controller::SampleRegistrationResult;
using Model::Sample;

namespace
{
    // ISampleRepository의 인메모리 페이크. 실제 파일 I/O 없이 SampleController 로직만 검증하기 위함.
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

    class SampleControllerTest : public ::testing::Test
    {
    protected:
        InMemorySampleRepository repository_;
    };

    // ---- 등록: 정상 케이스 ----

    TEST_F(SampleControllerTest, 유효한_입력으로_등록하면_Success를_반환한다)
    {
        SampleController controller(repository_);

        const auto result = controller.RegisterSample("S001", "Sample-A", 12.5, 0.9);

        EXPECT_EQ(result, SampleRegistrationResult::Success);
    }

    TEST_F(SampleControllerTest, 등록에_성공하면_GetAllSamples에_해당_시료가_포함된다)
    {
        SampleController controller(repository_);

        controller.RegisterSample("S001", "Sample-A", 12.5, 0.9);

        const auto all = controller.GetAllSamples();
        ASSERT_EQ(all.size(), 1u);
        EXPECT_EQ(all[0].GetSampleId(), "S001");
    }

    TEST_F(SampleControllerTest, 신규_등록된_시료의_초기_재고는_0이다)
    {
        SampleController controller(repository_);

        controller.RegisterSample("S001", "Sample-A", 12.5, 0.9);

        const auto all = controller.GetAllSamples();
        ASSERT_EQ(all.size(), 1u);
        EXPECT_EQ(all[0].GetStock(), 0);
    }

    // ---- 등록: 중복 ID ----

    TEST_F(SampleControllerTest, 중복된_시료ID로_등록하면_DuplicateSampleId를_반환한다)
    {
        SampleController controller(repository_);
        controller.RegisterSample("S001", "Sample-A", 12.5, 0.9);

        const auto result = controller.RegisterSample("S001", "Sample-B", 20.0, 0.5);

        EXPECT_EQ(result, SampleRegistrationResult::DuplicateSampleId);
    }

    TEST_F(SampleControllerTest, 중복된_시료ID로_등록해도_기존_시료는_변경되지_않는다)
    {
        SampleController controller(repository_);
        controller.RegisterSample("S001", "Sample-A", 12.5, 0.9);

        controller.RegisterSample("S001", "Sample-B", 20.0, 0.5);

        const auto all = controller.GetAllSamples();
        ASSERT_EQ(all.size(), 1u);
        EXPECT_EQ(all[0].GetName(), "Sample-A");
    }

    // ---- 등록: 유효성 검증 실패 ----

    TEST_F(SampleControllerTest, 시료ID가_비어있으면_InvalidInput을_반환한다)
    {
        SampleController controller(repository_);

        const auto result = controller.RegisterSample("", "Sample-A", 12.5, 0.9);

        EXPECT_EQ(result, SampleRegistrationResult::InvalidInput);
    }

    TEST_F(SampleControllerTest, 이름이_비어있으면_InvalidInput을_반환한다)
    {
        SampleController controller(repository_);

        const auto result = controller.RegisterSample("S001", "", 12.5, 0.9);

        EXPECT_EQ(result, SampleRegistrationResult::InvalidInput);
    }

    TEST_F(SampleControllerTest, 평균생산시간이_0이면_InvalidInput을_반환한다)
    {
        SampleController controller(repository_);

        const auto result = controller.RegisterSample("S001", "Sample-A", 0.0, 0.9);

        EXPECT_EQ(result, SampleRegistrationResult::InvalidInput);
    }

    TEST_F(SampleControllerTest, 평균생산시간이_음수면_InvalidInput을_반환한다)
    {
        SampleController controller(repository_);

        const auto result = controller.RegisterSample("S001", "Sample-A", -5.0, 0.9);

        EXPECT_EQ(result, SampleRegistrationResult::InvalidInput);
    }

    TEST_F(SampleControllerTest, 수율이_0이면_InvalidInput을_반환한다)
    {
        SampleController controller(repository_);

        const auto result = controller.RegisterSample("S001", "Sample-A", 12.5, 0.0);

        EXPECT_EQ(result, SampleRegistrationResult::InvalidInput);
    }

    TEST_F(SampleControllerTest, 수율이_음수면_InvalidInput을_반환한다)
    {
        SampleController controller(repository_);

        const auto result = controller.RegisterSample("S001", "Sample-A", 12.5, -0.1);

        EXPECT_EQ(result, SampleRegistrationResult::InvalidInput);
    }

    TEST_F(SampleControllerTest, 수율이_1보다_크면_InvalidInput을_반환한다)
    {
        SampleController controller(repository_);

        const auto result = controller.RegisterSample("S001", "Sample-A", 12.5, 1.1);

        EXPECT_EQ(result, SampleRegistrationResult::InvalidInput);
    }

    TEST_F(SampleControllerTest, 수율이_정확히_1이면_Success를_반환한다)
    {
        SampleController controller(repository_);

        const auto result = controller.RegisterSample("S001", "Sample-A", 12.5, 1.0);

        EXPECT_EQ(result, SampleRegistrationResult::Success);
    }

    TEST_F(SampleControllerTest, 유효성_검증에_실패하면_리포지토리에_추가되지_않는다)
    {
        SampleController controller(repository_);

        controller.RegisterSample("", "Sample-A", 12.5, 0.9);

        EXPECT_TRUE(controller.GetAllSamples().empty());
    }

    // ---- 전체 조회 ----

    TEST_F(SampleControllerTest, 등록된_시료가_없으면_GetAllSamples는_빈_목록을_반환한다)
    {
        SampleController controller(repository_);

        EXPECT_TRUE(controller.GetAllSamples().empty());
    }

    TEST_F(SampleControllerTest, 여러_시료를_등록하면_GetAllSamples는_등록한_개수만큼_반환한다)
    {
        SampleController controller(repository_);
        controller.RegisterSample("S001", "Sample-A", 12.5, 0.9);
        controller.RegisterSample("S002", "Sample-B", 20.0, 0.5);

        EXPECT_EQ(controller.GetAllSamples().size(), 2u);
    }

    // ---- 이름 검색 ----

    TEST_F(SampleControllerTest, 이름에_포함된_키워드로_검색하면_매치된다)
    {
        SampleController controller(repository_);
        controller.RegisterSample("S001", "Wafer Sample A", 12.5, 0.9);
        controller.RegisterSample("S002", "Chip Sample B", 20.0, 0.5);

        const auto matched = controller.SearchByName("Wafer");

        ASSERT_EQ(matched.size(), 1u);
        EXPECT_EQ(matched[0].GetSampleId(), "S001");
    }

    TEST_F(SampleControllerTest, 검색은_대소문자를_구분하지_않는다)
    {
        SampleController controller(repository_);
        controller.RegisterSample("S001", "Wafer Sample A", 12.5, 0.9);

        const auto matched = controller.SearchByName("wafer");

        EXPECT_EQ(matched.size(), 1u);
    }

    TEST_F(SampleControllerTest, 검색은_소문자_키워드로_대문자_이름도_찾는다)
    {
        SampleController controller(repository_);
        controller.RegisterSample("S001", "WAFER SAMPLE A", 12.5, 0.9);

        const auto matched = controller.SearchByName("wafer");

        EXPECT_EQ(matched.size(), 1u);
    }

    TEST_F(SampleControllerTest, 매치되는_시료가_없으면_빈_목록을_반환한다)
    {
        SampleController controller(repository_);
        controller.RegisterSample("S001", "Wafer Sample A", 12.5, 0.9);

        const auto matched = controller.SearchByName("NoSuchKeyword");

        EXPECT_TRUE(matched.empty());
    }

    TEST_F(SampleControllerTest, 빈_검색어로_검색하면_모든_시료가_매치된다)
    {
        SampleController controller(repository_);
        controller.RegisterSample("S001", "Wafer Sample A", 12.5, 0.9);
        controller.RegisterSample("S002", "Chip Sample B", 20.0, 0.5);

        const auto matched = controller.SearchByName("");

        EXPECT_EQ(matched.size(), 2u);
    }

    // ---- IsSampleRegistered ----

    TEST_F(SampleControllerTest, 등록된_시료ID는_IsSampleRegistered가_true를_반환한다)
    {
        SampleController controller(repository_);
        controller.RegisterSample("S001", "Sample-A", 12.5, 0.9);

        EXPECT_TRUE(controller.IsSampleRegistered("S001"));
    }

    TEST_F(SampleControllerTest, 등록되지_않은_시료ID는_IsSampleRegistered가_false를_반환한다)
    {
        SampleController controller(repository_);

        EXPECT_FALSE(controller.IsSampleRegistered("UNKNOWN"));
    }
}
