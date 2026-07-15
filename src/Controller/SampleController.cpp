#include "SampleController.h"

#include <algorithm>
#include <cctype>

namespace Controller
{
    namespace
    {
        constexpr double kMinimumAverageProductionMinutesPerUnit = 0.0;
        constexpr double kMinimumYield = 0.0;
        constexpr double kMaximumYield = 1.0;
        constexpr int kInitialStock = 0;

        std::string ToLowerCase(const std::string& text)
        {
            std::string lowered = text;
            std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
            return lowered;
        }

        bool ContainsIgnoreCase(const std::string& text, const std::string& keyword)
        {
            return ToLowerCase(text).find(ToLowerCase(keyword)) != std::string::npos;
        }
    }

    SampleController::SampleController(Persistence::ISampleRepository& sampleRepository)
        : sampleRepository_(sampleRepository)
    {
    }

    bool SampleController::IsValidRegistrationInput(
        const std::string& sampleId,
        const std::string& name,
        double averageProductionMinutesPerUnit,
        double yield)
    {
        if (sampleId.empty() || name.empty())
        {
            return false;
        }
        if (averageProductionMinutesPerUnit <= kMinimumAverageProductionMinutesPerUnit)
        {
            return false;
        }
        if (yield <= kMinimumYield || yield > kMaximumYield)
        {
            return false;
        }
        return true;
    }

    SampleRegistrationResult SampleController::RegisterSample(
        const std::string& sampleId,
        const std::string& name,
        double averageProductionMinutesPerUnit,
        double yield)
    {
        if (!IsValidRegistrationInput(sampleId, name, averageProductionMinutesPerUnit, yield))
        {
            return SampleRegistrationResult::InvalidInput;
        }

        const Model::Sample newSample(sampleId, name, averageProductionMinutesPerUnit, yield, kInitialStock);

        if (!sampleRepository_.Create(newSample))
        {
            return SampleRegistrationResult::DuplicateSampleId;
        }
        return SampleRegistrationResult::Success;
    }

    std::vector<Model::Sample> SampleController::GetAllSamples() const
    {
        return sampleRepository_.GetAll();
    }

    std::vector<Model::Sample> SampleController::SearchByName(const std::string& keyword) const
    {
        std::vector<Model::Sample> matchedSamples;
        for (const auto& sample : sampleRepository_.GetAll())
        {
            if (ContainsIgnoreCase(sample.GetName(), keyword))
            {
                matchedSamples.push_back(sample);
            }
        }
        return matchedSamples;
    }

    bool SampleController::IsSampleRegistered(const std::string& sampleId) const
    {
        return sampleRepository_.FindById(sampleId).has_value();
    }

    std::optional<Model::Sample> SampleController::FindSampleById(const std::string& sampleId) const
    {
        return sampleRepository_.FindById(sampleId);
    }
}
