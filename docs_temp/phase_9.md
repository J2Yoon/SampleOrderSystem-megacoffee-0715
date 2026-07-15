# Phase 9 — 메인 메뉴 통합

## 관련 문서
- `docs/All_phase_goals.md` Phase 9
- `docs/PRD.md` 4.1(메인 메뉴)

## 목표
지금까지 만든 기능(시료 관리/주문 접수/승인·거절/생산 라인/출고/모니터링)을 하나의 메인 메뉴로 연결하고,
시작 시 요약 정보와 생산 큐 정산을 수행한다.

## 설계 결정

1. **MainMenuController** (`Controller/`): 요약 정보(`MainMenuSummary{registeredSampleCount, totalStock,
   totalOrderCount, productionQueuePendingItemCount}`)만 계산하는 얇은 Controller. `<iostream>` 의존 없음.
   - 생산라인 대기 건수를 위해 `ProductionLineController::GetPendingItemCount()`를 신규 추가한다
     (큐 저장소에 남아있는 전체 항목 수 = 대기 중 + 현재 생산 중; `SettleCompletedItems` 이후 호출 전제).
2. **MainMenuView** (`View/`): 다른 View들과 달리, Controller가 아니라 이미 조립된 하위 View들
   (`SampleView, OrderView, OrderApprovalView, ProductionLineView, ShipmentView, MonitoringView`)에 대한
   참조와 `MainMenuController`를 받아 최상위 메뉴 루프(`Run()`)를 담당한다. Controller는 View를 모르므로
   (아키텍처 규칙), 하위 화면 라우팅은 Controller가 아닌 View 계층에 둔다.
   - 메뉴 진입 시마다 요약 정보를 먼저 출력([1]~[6] 메뉴 이전).
   - 메뉴: `[1] 시료 관리 [2] 주문 접수 [3] 주문 승인/거절 [4] 생산 라인 [5] 출고 처리 [6] 모니터링 [0] 종료`.
3. **main.cpp**: 기존 DI 조립 순서 유지 + `MonitoringController/View`, `MainMenuController/View` 추가 조립.
   `productionLineController.SettleCompletedItems()`는 `MainMenuView::Run()` 이전에 그대로 호출 유지.
   기존 main.cpp의 인라인 메뉴 루프는 `MainMenuView::Run()` 호출로 대체한다.

## 산출물 파일 목록
- `src/Controller/MainMenuController.h` / `.cpp` (신규)
- `src/View/MainMenuView.h` / `.cpp` (신규)
- `src/Controller/ProductionLineController.h` / `.cpp` (수정 — `GetPendingItemCount()` 추가)
- `src/main.cpp` (수정 — 전체 배선 + `MainMenuView::Run()`으로 대체)
- `SampleOrderSystem.vcxproj` (신규 파일 등록)

## 완료 조건과의 매핑
- 시작 시 요약 정보 표시 -> `MainMenuView::Run()` 루프 상단에서 매번 `MainMenuController::GetSummary()` 출력.
- 시작 시 생산 큐 정산 선행 -> `main.cpp`에서 `SettleCompletedItems()`를 `MainMenuView::Run()` 이전에 호출.
- 전체 시나리오 수동 검증 -> 통합된 메뉴에서 6개 하위 기능 모두 라우팅됨.

## 재작업 이력
(초기 구현, 아직 unit-tester 피드백 없음)
