# Phase 6 — 생산 라인 (FIFO 큐 + 시간 기반 완료 처리)

## 관련 문서
- `docs/All_phase_goals.md` Phase 6 (생산 라인 — FIFO 큐 + 시간 기반 완료 처리)
- `docs/PRD.md` 4.6(생산 라인) 전체, 특히 4.6.1(생산량/생산시간 계산, 재고 확인/갱신 시점 표),
  4.6.2(생산 현황 표기), 4.6.3(대기 주문 확인 — FIFO), 4.6.4(실시간성/영속성 요구사항),
  5.3(생산 큐 항목 데이터 모델)
- `CLAUDE.md` 아키텍처 절(Controller/View 분리, Controller는 `<iostream>` 비의존), 핵심 도메인 규칙
  ("생산 라인은 단일 라인이며 FIFO", "실 생산량/총 생산 시간 계산식", "생산 시간은 실제 경과 시간을 반영",
  "재고의 물리적 증감은 생산 완료 시/출고 시 두 시점"), Clean Code 원칙(함수화, DRY)

## 목표
Phase 5까지는 재고 부족 시 `ProductionQueueItem`을 생산 큐에 "등록"만 했다. Phase 6은 이 큐를 단일 FIFO
라인으로 실제 "처리"한다: 앱을 실행/재시작한 시점의 현재 시각을 기준으로, 이미 완료됐어야 할 큐 항목을
일괄 정산(재고 증가 + `PRODUCING → CONFIRMED` 전환 + 큐에서 제거)하고, 아직 진행 중이거나 대기 중인 항목을
조회할 수 있게 한다. 단순 인메모리 타이머가 아니라 저장된 시작 시각과 현재 시각의 차이로 정산하므로,
앱이 꺼져 있던 동안에도 생산이 계속 진행된 것으로 간주된다(PRD 4.6.4).

## 기존 구조와의 연결
- `Model::ProductionQueueItem`(Phase 2)이 이미 `productionStartTime`/`totalProductionTime`/
  `expectedCompletionTime`을 보유하고 있다. 다만 Phase 5의 설계 결정 4에 따르면 `productionStartTime`은
  "승인 시각"을 그대로 저장했을 뿐, 앞선 큐 항목이 아직 생산 중이라 실제로는 더 늦게 시작해야 하는
  FIFO 대기 지연은 반영되지 않았다("실제 FIFO 대기로 인한 시작 지연 반영은 Phase 6의 몫"). 따라서 이번
  Phase는 이 지연을 큐 순서대로 누적 계산(cascading)하는 것이 핵심이다.
- `Persistence::IProductionQueueRepository::GetAll()`은 이미 "파일에 기록된 순서 = FIFO 순서"를
  보장하도록 문서화되어 있다(Phase 2). 새로 정렬 로직을 만들 필요가 없다.
- `Controller::OrderApprovalController`가 승인 시점에 실 생산량/총 생산 시간을 계산해 큐 항목을
  생성하는 로직(`CalculateActualProductionQuantity`/`CalculateTotalProductionTime`, private static)을
  이미 갖고 있다. Phase 6 목표 문서도 동일한 계산식을 `ProductionLineController`의 산출물로 명시하고
  있으므로, 두 Controller가 같은 공식을 중복 구현하지 않도록 공용 계산 유틸리티로 추출한다(DRY).
- `Model::Sample`은 아직 재고를 변경하는 Controller 메서드가 없다(`SetStock`만 Model에 존재하고,
  "실제 증감 로직은 Controller 계층(Phase 6/7)이 담당한다"는 주석이 이미 달려 있음). 이번 Phase에서
  `Controller::SampleController::IncreaseStock`을 신설한다.

## 산출물 목록

| 파일 | 내용 | 완료 조건 매핑 |
|---|---|---|
| `src/Controller/ProductionCalculator.h/.cpp`(신규) | `CalculateActualProductionQuantity(shortageQuantity, yield)`, `CalculateTotalProductionTime(averageProductionMinutesPerUnit, actualProductionQuantity)` 순수 계산 함수(부작용 없음). `OrderApprovalController`와 `ProductionLineController`가 공유 | DoD "실 생산량/총 생산 시간 계산식에 대한 단위 테스트(ceil 처리 포함)" — 단일 구현으로 양쪽에서 검증 가능 |
| `src/Controller/OrderApprovalController.h/.cpp`(수정) | 기존 private static `CalculateActualProductionQuantity`/`CalculateTotalProductionTime`을 제거하고 `ProductionCalculator::...` 호출로 대체(동작 변경 없음, 순수 리팩터링) | Phase 5 기존 테스트 회귀 없음 유지, DRY |
| `src/Controller/SampleController.h/.cpp`(수정) | `IncreaseStock(sampleId, increaseAmount) -> bool` 신설. 대상 시료가 없으면 false, 있으면 재고를 더한 뒤 `Update` | PRD 4.6.1 "생산 완료 시 실 생산량만큼 재고를 실제로 증가" |
| `src/Controller/ProductionLineController.h/.cpp`(신규) | `ProductionLineStatus`(현재 생산 중 항목 + 실제 시작/완료 시각) 값 구조체. `ProductionLineController` 클래스: `SettleCompletedItems(now = 현재시각)`(정산된 건수 반환), `GetCurrentProductionStatus()`(큐 맨 앞 항목, 없으면 `nullopt`), `GetWaitingQueue()`(맨 앞을 제외한 나머지 FIFO 목록). 생성자는 `IProductionQueueRepository&`, `IOrderRepository&`, `SampleController&`를 주입받음 | DoD 전체(계산식, 시간 정산, 재고 증가+상태 전환) |
| `src/View/ProductionLineView.h/.cpp`(신규) | 생산 라인 서브메뉴(현재 생산 현황 / 대기 큐 목록 / 뒤로) 표시. 메뉴 진입 시마다 먼저 `SettleCompletedItems()`를 호출해 화면이 항상 최신 경과 시간을 반영하게 함 | PRD 4.6.2/4.6.3 표기, "표시할 때마다 실제 경과 시간 반영" |
| `src/main.cpp`(수정, Release 분기) | `ProductionLineController`/`ProductionLineView` 조립, 앱 시작 시 1회 `SettleCompletedItems()` 호출(재시작 시점 정산), 상위 메뉴에 "[4] 생산 라인" 추가 | PRD 4.6.4 "재시작 시점 현재 시각 기준 일괄 정산" |
| `SampleOrderSystem.vcxproj` / `.vcxproj.filters`(수정) | 신규 소스 6개(ProductionCalculator, ProductionLineController, ProductionLineView 각 .h/.cpp) 등록 | 빌드 포함 |

## 설계 결정

### 1. FIFO 단일 라인의 "실제" 시작/완료 시각을 큐 순서로 누적 계산(cascading schedule)
`ProductionQueueItem.productionStartTime`은 승인 시각일 뿐, 앞선 항목이 아직 생산 중이면 실제 생산은
그보다 늦게 시작한다. 단일 라인이므로 항목 i의 실제 시작 시각은
`max(항목 i의 productionStartTime, 항목 i-1의 실제 완료 시각)`이고, 실제 완료 시각은
`실제 시작 시각 + totalProductionTime`이다. `ProductionLineController::BuildSchedule()`(private)이
`productionQueueRepository_.GetAll()`(이미 FIFO 순서)을 순회하며 이 누적 값을 계산해
`std::vector<ProductionLineStatus>`로 반환한다. 실제 완료 시각은 큐 순서상 항상 비내림차순이므로,
정산 시 앞에서부터 순회하다 아직 완료되지 않은 첫 항목을 만나면 그 뒤는 볼 필요 없이 즉시 멈출 수 있다.

### 2. `SettleCompletedItems`는 스냅샷 기반으로 안전하게 처리
`BuildSchedule()`이 저장소의 복사본(`std::vector`)을 만들어 반환하므로, 정산 루프 안에서
`productionQueueRepository_.Remove(...)`를 호출해도 순회 중인 컨테이너가 아니라 별도 스냅샷을 순회하는
것이라 반복자 무효화 문제가 없다.

### 3. 정산 시 주문을 찾을 수 없는 경우의 방어적 처리
`ProductionQueueItem`은 `orderId`만 들고 있으므로, 재고를 증가시킬 시료 ID는 연결된 `Order`를 통해
얻어야 한다(`orderRepository_.FindById(item.GetOrderId())`). 정상 흐름에서는 `OrderApprovalController`가
주문과 큐 항목을 항상 함께 만들고 이 Phase가 유일하게 큐 항목을 지우는 주체이므로 주문이 사라지는 일은
없어야 하지만, 향후 주문 삭제 기능이 생길 가능성에 대비해 주문을 찾지 못하면 재고 증가/상태 전환은
건너뛰고 큐 항목만 제거한다(무한 대기 방지, Phase 5의 `SampleNotFound` 방어 패턴과 동일한 철학).

### 4. 계산식 공용화 — `Controller::ProductionCalculator`
Phase 6 목표 문서가 "실 생산량/총 생산 시간 계산"을 `ProductionLineController`의 산출물로 명시하지만,
동일한 공식이 이미 Phase 5의 `OrderApprovalController`(큐 항목 생성 시점)에 구현되어 있다. 새 Controller에
같은 코드를 다시 쓰면 DRY 위반이자 두 곳의 계산이 미묘하게 갈릴 위험이 생기므로, 부작용 없는 순수 함수
2개를 `Controller::ProductionCalculator`(네임스페이스, 클래스 아님 — 상태가 없고 순수 계산이라 클래스로
감쌀 이유가 없음)로 추출해 양쪽에서 재사용한다. `OrderApprovalController`의 기존 private static 메서드는
제거하고 새 공용 함수 호출로 대체한다(동작/서명 변경 없음, 순수 리팩터링이라 Phase 5 테스트에 영향 없음 —
해당 계산 메서드는 애초에 private라 직접 테스트된 적이 없고 `ApproveOrder`의 관찰 가능한 동작을 통해서만
검증되었음을 확인함).

### 5. `GetCurrentProductionStatus`/`GetWaitingQueue`는 정산 이후 호출을 전제
두 조회 메서드는 그 자체로 시간 정산을 하지 않는다(단일 책임 분리 — 정산은 `SettleCompletedItems`만
담당). `View::ProductionLineView`가 메뉴 진입 시마다 먼저 `SettleCompletedItems()`를 호출한 뒤 조회
메서드를 사용하도록 순서를 강제한다. `main.cpp`도 앱 시작 직후 1회 호출해 "재시작 시점 정산"을 보장한다.

### 6. `SampleController::IncreaseStock`은 `ProductionLineController`만 호출(대칭성)
`OrderApprovalController`는 재고를 조회만 하는 `const SampleController&`를 주입받아 물리적 차감이
불가능하도록 타입 수준에서 강제했다(Phase 5). 동일한 원칙으로 `ProductionLineController`는 재고를
실제로 증가시켜야 하므로 비-const `SampleController&`를 주입받는다. `main.cpp`의 `sampleController`는
이미 비-const이므로 두 Controller 모두에 정상적으로 전달 가능하다.

## 완료 조건(DoD) 매핑
- 실 생산량/총 생산 시간 계산식에 대한 단위 테스트(수율에 따른 ceil 처리 포함) 통과:
  `Controller::ProductionCalculator::CalculateActualProductionQuantity`/`CalculateTotalProductionTime`을
  직접 테스트 가능한 공개 함수로 제공.
- 앱 재시작을 시뮬레이션했을 때(저장된 시작 시각 기준 경과 시간 계산) 완료됐어야 할 큐 항목이 올바르게
  정산되는 테스트 통과: `SettleCompletedItems(now)`에 임의의 `now`(예: 미래 시각)를 주입할 수 있게
  해 "앱이 오래 꺼져 있다 재시작"하는 상황을 테스트에서 재현 가능.
- 생산 완료 시 재고가 실 생산량만큼 증가하고 주문이 CONFIRMED로 전환됨을 확인하는 테스트 통과:
  `CompleteProductionQueueItem`이 `sampleController_.IncreaseStock(order.GetSampleId(), item.GetActualProductionQuantity())`
  호출 후 `order.TryTransitionTo(Confirmed)` + `orderRepository_.Update(order)`를 수행.

## 빌드 반영
- `SampleOrderSystem.vcxproj`/`.vcxproj.filters`에 신규 소스 6개(ProductionCalculator, ProductionLineController,
  ProductionLineView 각 `.h`/`.cpp`) 추가.
- `src/main.cpp`의 Release 분기 수정(`ProductionLineController`/`ProductionLineView` 조립, 시작 시
  `SettleCompletedItems()` 1회 호출, 상위 메뉴 "[4] 생산 라인" 추가).
- Debug(x64)/Release(x64) 빌드 결과는 구현 직후 별도로 기록(아래 "빌드 확인 결과" 참고).

## 다음으로 unit-tester가 검증할 대상
- `Controller::ProductionCalculator::CalculateActualProductionQuantity`: 나누어떨어지는 경우/나머지가
  남는 경우 각각의 `ceil` 처리, 수율 1.0(나머지 없음 보장) 등.
- `Controller::ProductionCalculator::CalculateTotalProductionTime`: 단순 곱셈이지만 소수점 평균
  생산시간에 대한 결과 확인.
- `Controller::ProductionLineController::SettleCompletedItems`
  - 큐가 비어 있으면 0건 정산.
  - 단일 항목이 아직 완료 시각 이전이면 0건 정산, 상태/재고 불변.
  - 단일 항목이 완료 시각을 지났으면 1건 정산 + 재고가 `actualProductionQuantity`만큼 증가 + 연결 주문이
    `PRODUCING → CONFIRMED`로 전환 + 큐에서 제거됨.
  - 큐에 2건 이상 있고, 두 번째 항목의 `productionStartTime`(승인 시각) 기준으로는 이미 지났어야 하지만
    첫 번째 항목이 아직 진행 중이라 실제로는 아직 시작도 안 했어야 하는 경우(FIFO 지연) — 두 번째 항목은
    정산되지 않아야 함(누적 스케줄 계산 검증의 핵심 케이스).
  - `now`를 아주 먼 미래로 주면 큐의 모든 항목이 순서대로 정산됨.
  - 큐 항목이 가리키는 주문을 찾을 수 없는 경우 방어적으로 큐 항목만 제거되고 예외가 발생하지 않는지.
- `Controller::ProductionLineController::GetCurrentProductionStatus`/`GetWaitingQueue`: 큐가 비어있을 때
  `nullopt`/빈 벡터, 여러 항목이 있을 때 첫 번째와 나머지가 올바르게 분리되는지, FIFO 순서 유지 확인.
- `Controller::SampleController::IncreaseStock`: 존재하는/존재하지 않는 시료 ID 각각에 대한 반환값과
  재고 반영 여부.

## 재작업 이력
(현재까지 없음)
