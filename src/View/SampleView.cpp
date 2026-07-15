#include "SampleView.h"

#include <iomanip>
#include <iostream>
#include <string>

#include "ConsoleView.h"

namespace View
{
    namespace
    {
        constexpr int kMenuChoiceRegister = 1;
        constexpr int kMenuChoiceListAll = 2;
        constexpr int kMenuChoiceSearch = 3;
        constexpr int kMenuChoiceBack = 0;
    }

    SampleView::SampleView(Controller::SampleController& sampleController)
        : sampleController_(sampleController)
    {
    }

    bool SampleView::ShowMenuAndHandleSelection()
    {
        ConsoleView::PrintTitle("[1] 시료 관리");
        ConsoleView::PrintLine("[1] 시료 등록   [2] 시료 목록   [3] 시료 검색   [0] 뒤로");
        const int choice = ConsoleView::ReadInt("선택 > ");
        ConsoleView::PrintLine();

        switch (choice)
        {
        case kMenuChoiceRegister:
            HandleRegister();
            return true;
        case kMenuChoiceListAll:
            HandleListAll();
            return true;
        case kMenuChoiceSearch:
            HandleSearch();
            return true;
        case kMenuChoiceBack:
            return false;
        default:
            ConsoleView::PrintError("올바른 번호를 선택해주세요.");
            return true;
        }
    }

    void SampleView::HandleRegister()
    {
        const std::string sampleId = ConsoleView::ReadLine("시료 ID   > ");
        const std::string name = ConsoleView::ReadLine("시료명    > ");
        const double averageProductionMinutesPerUnit = ConsoleView::ReadDouble("평균 생산시간(분/ea) > ");
        const double yield = ConsoleView::ReadDouble("수율(0.0~1.0)        > ");

        const auto result = sampleController_.RegisterSample(
            sampleId, name, averageProductionMinutesPerUnit, yield);

        switch (result)
        {
        case Controller::SampleRegistrationResult::Success:
            ConsoleView::PrintLine("시료 등록 완료.");
            break;
        case Controller::SampleRegistrationResult::DuplicateSampleId:
            ConsoleView::PrintError("이미 등록된 시료 ID 입니다.");
            break;
        case Controller::SampleRegistrationResult::InvalidInput:
            ConsoleView::PrintError(
                "입력값을 확인해주세요(ID/이름은 비어있을 수 없고, 평균 생산시간은 0보다 커야 하며, "
                "수율은 0 초과 1 이하여야 합니다).");
            break;
        }
    }

    void SampleView::HandleListAll() const
    {
        const auto samples = sampleController_.GetAllSamples();
        ConsoleView::PrintLine("등록 시료 목록 (총 " + std::to_string(samples.size()) + "종)");
        PrintSampleTableHeader();
        for (const auto& sample : samples)
        {
            PrintSampleRow(sample);
        }
    }

    void SampleView::HandleSearch() const
    {
        const std::string keyword = ConsoleView::ReadLine("검색어(이름 포함) > ");
        const auto samples = sampleController_.SearchByName(keyword);
        ConsoleView::PrintLine("검색 결과 " + std::to_string(samples.size()) + "건");
        PrintSampleTableHeader();
        for (const auto& sample : samples)
        {
            PrintSampleRow(sample);
        }
    }

    void SampleView::PrintSampleTableHeader()
    {
        std::cout << std::left << std::setw(10) << "ID" << std::setw(20) << "시료명"
            << std::setw(16) << "평균생산시간\t" << std::setw(16) << "수율" << "재고\n";
    }

    void SampleView::PrintSampleRow(const Model::Sample& sample)
    {
        std::cout << std::left << std::setw(10) << sample.GetSampleId() << std::setw(20) << sample.GetName()
            << std::setw(16) << sample.GetAverageProductionMinutesPerUnit() << std::setw(10) << sample.GetYield()
            << sample.GetStock() << " ea\n";
    }
}
