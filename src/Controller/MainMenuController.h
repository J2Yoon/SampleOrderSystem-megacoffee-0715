#pragma once

#include "../Persistence/IOrderRepository.h"
#include "ProductionLineController.h"
#include "SampleController.h"

namespace Controller
{
    // 앱 시작(및 메인 메뉴 진입) 시 표시할 요약 정보(docs/PRD.md 4.1).
    struct MainMenuSummary
    {
        int registeredSampleCount = 0;
        int totalStock = 0;
        int totalOrderCount = 0;
        int productionQueuePendingItemCount = 0;
    };

    // 메인 메뉴 요약 정보 계산을 담당하는 Controller. docs/PRD.md 4.1 참고.
    // 콘솔 입출력(<iostream>)에 의존하지 않으며, 하위 화면으로의 라우팅은 View 계층(MainMenuView)이 담당한다
    // (Controller는 View를 알지 못한다는 아키텍처 규칙에 따름).
    class MainMenuController
    {
    public:
        MainMenuController(
            SampleController& sampleController,
            Persistence::IOrderRepository& orderRepository,
            ProductionLineController& productionLineController);

        // 등록 시료 수, 총 재고, 전체 주문 수, 생산라인 대기 건수를 계산해 반환한다.
        MainMenuSummary GetSummary() const;

    private:
        SampleController& sampleController_;
        Persistence::IOrderRepository& orderRepository_;
        ProductionLineController& productionLineController_;
    };
}
