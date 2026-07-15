#pragma once

#include "../Controller/ProductionLineController.h"

namespace View
{
    // 생산 라인 메뉴(현재 생산 현황 / 대기 큐 목록)의 콘솔 입출력을 담당하는 View.
    // 도메인 로직을 갖지 않으며, 사용자 입력을 ProductionLineController에 위임하고 그 결과만 표시한다.
    class ProductionLineView
    {
    public:
        explicit ProductionLineView(Controller::ProductionLineController& productionLineController);

        // 생산 라인 메뉴를 한 번 표시하고 선택 항목을 처리한다.
        // 메뉴 진입 시마다 먼저 실제 경과 시간을 기준으로 큐 정산을 수행해 화면이 항상 최신 상태를
        // 반영하도록 한다(docs/PRD.md 4.6.4).
        // 사용자가 "뒤로(0)"를 선택하면 false, 그 외에는 true를 반환한다.
        bool ShowMenuAndHandleSelection();

    private:
        void HandleShowCurrentStatus() const;
        void HandleShowWaitingQueue() const;

        static void PrintProductionQueueItemRow(const Model::ProductionQueueItem& item);

        Controller::ProductionLineController& productionLineController_;
    };
}
