# Phase 8 — 모니터링

## 관련 문서
- `docs/All_phase_goals.md` Phase 8
- `docs/PRD.md` 4.5(모니터링)

## 목표
상태별 주문 현황(REJECTED 제외)과 시료별 재고 현황(여유/부족/고갈)을 조회한다.

## 설계 결정

1. **MonitoringService**: `Controller/MonitoringService.h/.cpp`에 `ProductionCalculator`(Phase 6)와 동일하게
   순수 계산 네임스페이스로 둔다(파일 접근 없음, 이미 로드된 `Sample`/`Order` 벡터만 입력받음).
   - `CountOrdersByStatus`: `Reserved/Producing/Confirmed/Released` 4개 키만 갖는 `map`을 반환하고
     `Rejected`는 집계에서 제외한다(DataMonitor `MonitoringService::CountOrdersByStatus` 참고).
   - `StockLevel`(Sufficient/Low/Depleted), `StockStatus{sample, pendingDemand, level}`을 `Controller`
     네임스페이스에 정의한다. `pendingDemand`는 해당 시료의 `RESERVED/PRODUCING/CONFIRMED` 주문 수량 합.
     재고 0 -> Depleted, 재고 < pendingDemand -> Low, 그 외 -> Sufficient.
2. **MonitoringController**: `IOrderRepository` + `SampleController`를 참조해 `MonitoringService`를 호출하는
   얇은 래퍼(`ShipmentController`와 동일한 얇은 위임 패턴). 콘솔 I/O 없음.
3. **MonitoringView**: `[1] 상태별 주문 현황 [2] 시료별 재고 현황 [0] 뒤로` 메뉴. 주문 현황은 PRD 순서
   (RESERVED/CONFIRMED/PRODUCING/RELEASED)대로 출력. `StockLevel` -> 한글 표시 문자열 변환은 View 전담.

## 산출물 파일 목록
- `src/Controller/MonitoringService.h` / `.cpp` (신규)
- `src/Controller/MonitoringController.h` / `.cpp` (신규)
- `src/View/MonitoringView.h` / `.cpp` (신규)
- `SampleOrderSystem.vcxproj` (신규 파일 등록)

## 완료 조건과의 매핑
- `REJECTED` 제외 -> `CountOrdersByStatus`가 `Rejected` 키를 아예 만들지 않음.
- 재고 상태 판정 -> `BuildStockStatuses`의 Depleted/Low/Sufficient 분기.

## 재작업 이력
(초기 구현, 아직 unit-tester 피드백 없음)
