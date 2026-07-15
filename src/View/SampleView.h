#pragma once

#include "../Controller/SampleController.h"

namespace View
{
    // 시료 관리(등록/조회/검색) 메뉴의 콘솔 입출력을 담당하는 View.
    // 도메인 로직을 갖지 않으며, 사용자 입력을 SampleController에 위임하고 그 결과만 표시한다.
    class SampleView
    {
    public:
        explicit SampleView(Controller::SampleController& sampleController);

        // 시료 관리 메뉴를 한 번 표시하고 선택 항목을 처리한다.
        // 사용자가 "뒤로(0)"를 선택하면 false, 그 외에는 true를 반환한다.
        bool ShowMenuAndHandleSelection();

    private:
        void HandleRegister();
        void HandleListAll() const;
        void HandleSearch() const;

        static void PrintSampleTableHeader();
        static void PrintSampleRow(const Model::Sample& sample);

        Controller::SampleController& sampleController_;
    };
}
