#ifdef _DEBUG

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#else

#include "Controller/SampleController.h"
#include "Persistence/JsonSampleRepository.h"
#include "View/ConsoleView.h"
#include "View/SampleView.h"

// Phase 9(메인 메뉴 통합)에서 MainMenuController/View로 대체될 임시 진입점이다.
// 현재는 Phase 3까지 구현된 시료 관리 기능만 콘솔에서 수동으로 검증할 수 있게 배선한다.
int main()
{
    Persistence::JsonSampleRepository sampleRepository("data/samples.json");
    Controller::SampleController sampleController(sampleRepository);
    View::SampleView sampleView(sampleController);

    View::ConsoleView::PrintTitle("반도체 시료 생산주문관리 시스템 (Phase 3 - 시료 관리)");
    View::ConsoleView::PrintLine("주문/승인/생산/출고/모니터링 메뉴는 이후 Phase에서 통합될 예정입니다.");

    while (sampleView.ShowMenuAndHandleSelection())
    {
    }

    return 0;
}

#endif
