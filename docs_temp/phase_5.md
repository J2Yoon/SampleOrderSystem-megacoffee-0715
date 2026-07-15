# Phase 5 — 주문 승인/거절 (재고 판단 로직)

## 관련 문서
- `docs/All_phase_goals.md` Phase 5 (주문 승인/거절 — 재고 판단 로직)
- `docs/PRD.md` 3(주문 상태 정의), 4.4(주문 승인/거절), 4.6.1(재고 확인/갱신 시점 표 — 승인 시점은 "조회"만),
  5.3(생산 큐 항목 데이터 모델)
- `CLAUDE.md` 아키텍처 절(Controller/View 분리, Controller는 `<iostream>` 비의존), 핵심 도메인 규칙("주문
  승인 시 재고 판단"/"재고의 물리적 증감은 두 시점", "생산 라인은 단일(1개) 라인이며 FIFO"), Clean Code
  원칙("생성 규칙이 복잡한 객체(승인 시점에 파생되는 `ProductionQueueItem` 생성)는 Factory Method 패턴 고려")
- 워크플로우 변경 안내: 이번 Phase부터 doc-verifier/spec-verifier 없이 phase-developer ↔ unit-tester 2단계
  루프만 진행한다(`docs/AGENTS.md`, `CLAUDE.md` 최신 반영 기준).

## 목표
Phase 4까지 `RESERVED` 상태로만 접수되던 주문을, 재고 조회 결과에 따라 자동으로 `CONFIRMED`(재고 충분) 또는
`PRODUCING`(재고 부족 + 생산 큐 등록)으로 전환하거나, 담당자가 명시적으로 `REJECTED`로 거절할 수 있게 한다.
재고는 이 시점에 물리적으로 차감하지 않고 조회만 한다(물리적 증감은 Phase 6 생산 완료 시(+), Phase 7 출고
시(-)에만 발생). 동일 시료에 대한 여러 승인 건은 각각 독립된 생산 큐 항목으로 등록하며 병합하지 않는다
(`Persistence::JsonProductionQueueRepository::Create`가 이미 주문번호 키 중복 시 false를 반환하도록
Phase 2에서 구현되어 있어, 서로 다른 주문번호는 자동으로 별도 항목이 된다).

## 산출물 목록

| 파일 | 내용 | 완료 조건 매핑 |
|---|---|---|
| `src/Controller/SampleController.h/.cpp`(수정) | `FindSampleById(sampleId) const -> std::optional<Model::Sample>` 공개 메서드 추가. 기존 `IsSampleRegistered`와 동일하게 `sampleRepository_.FindById`에 위임하되, 승인 로직이 재고 값을 조회할 수 있도록 시료 전체(재고 포함)를 반환 | PRD 4.6.1 "승인 시점 재고 조회"를 위해 `OrderApprovalController`가 시료 데이터를 얻는 유일한 경로 |
| `src/Controller/OrderApprovalController.h/.cpp`(신규) | `OrderApprovalResult`/`OrderRejectionResult` enum, `OrderApprovalController` 클래스: `GetReservedOrders()`, `ApproveOrder(orderId, approvedAt = now())`, `RejectOrder(orderId)`. 생성자는 `Persistence::IOrderRepository&`, `const Controller::SampleController&`, `Persistence::IProductionQueueRepository&`를 주입받는다 | PRD 4.4, DoD "재고 충분/부족 각 케이스 상태 전이", "승인 시점 재고 미차감", "REJECTED 이후 전이 차단", "병합 없는 독립 큐 항목" |
| `src/View/OrderApprovalView.h/.cpp`(신규) | 승인/거절 서브메뉴(접수 대기 목록 조회 / 승인 / 거절 / 뒤로) 표시 및 입력 위임. 도메인 로직 없이 `OrderApprovalController` 호출 결과만 표시 | CLAUDE.md "View는 도메인 로직을 갖지 않는다" |
| `src/main.cpp`(수정, Release 분기) | `JsonProductionQueueRepository` 추가, `OrderApprovalController`(orderRepository, sampleController, productionQueueRepository) → `OrderApprovalView` 조립. 상위 메뉴에 "[3] 주문 승인/거절" 추가 | 수동 실행으로 접수 → 승인/거절 흐름 확인 가능 |
| `SampleOrderSystem.vcxproj` / `.vcxproj.filters`(수정) | 신규 소스 4개(OrderApprovalController.h/.cpp, OrderApprovalView.h/.cpp) 등록 | 빌드 포함 |

## 설계 결정

### 1. 별도 Controller로 분리(`OrderController`에 메서드 추가하지 않음)
`OrderController`는 "주문 접수(예약)"라는 단일 책임을 갖고 있고(Phase 4), 승인/거절은 재고 조회·생산 큐 등록
이라는 별개의 책임과 의존성(SampleController의 재고 조회, IProductionQueueRepository)을 추가로 필요로 한다.
하나의 Controller에 접수/승인/거절을 모두 넣으면 생성자가 필요 이상의 의존성(생산 큐 저장소)을 항상 요구하게
되어 "주문 접수만 하는" 테스트/사용처에서도 불필요한 결합이 생긴다. SRP를 지키기 위해
`Controller::OrderApprovalController`로 분리한다(ConsoleMVC의 "Controller는 기능 단위로 분리" 관례와도 일치).

### 2. 재고 조회 경로: `SampleController::FindSampleById` 신설
Phase 4 설계 결정 1과 동일한 원칙(컨트롤러 간 협력, `ISampleRepository`를 직접 재주입하지 않음)을 따른다.
`OrderApprovalController`는 `const Controller::SampleController& sampleController_`만 주입받고, 새로 추가하는
`SampleController::FindSampleById`를 통해 시료(재고 포함)를 조회한다. 재고를 변경하지 않으므로 `const&`로
주입해 타입 수준에서 "승인 처리는 재고를 변경할 수 없다"는 규칙을 보장한다.

### 3. 재고 판단 로직과 상태 분기
```
shortageQuantity = order.GetQuantity() - sample.GetStock()   // PRD 7절 용어 정의: 부족분 = 주문수량 - 승인 시점 재고
if (shortageQuantity <= 0):
    order.TryTransitionTo(Confirmed)   // 재고 충분 → 즉시 출고 대기
else:
    productionQueueItem = 생성(shortageQuantity, sample.GetYield(), sample.GetAverageProductionMinutesPerUnit(), approvedAt)
    productionQueueRepository_.Create(productionQueueItem)
    order.TryTransitionTo(Producing)   // 재고 부족 → 생산 큐 등록
orderRepository_.Update(order)
```
- 재고가 정확히 0 부족(`shortageQuantity == 0`)인 경우도 "충분"으로 취급한다(PRD 4.4.2 "부족분 = 주문 수량 −
  현재 재고", 부족분이 0이면 추가 생산이 필요 없으므로 `CONFIRMED`가 맞다).
- 재고를 직접 차감하지 않는다는 규칙(PRD 4.6.1)을 지키기 위해 `sample.SetStock(...)`을 호출하지 않고,
  `sample.GetStock()`으로 읽기만 한다. `SampleController`에도 재고 변경 메서드를 호출하지 않는다.

### 4. `ProductionQueueItem` 생성 — Factory Method 패턴(과설계 방지 수준)
CLAUDE.md Clean Code 원칙에서 "승인 시점에 파생되는 `ProductionQueueItem` 생성"을 Factory Method 후보로
명시하고 있다. 별도 클래스(`ProductionQueueItemFactory`)까지 두면 이 Phase의 복잡도에 비해 과설계이므로,
`OrderApprovalController`의 private static 메서드로 아래처럼 계산 단계를 함수로 쪼개 이름을 붙인다(Clean Code
"함수화" 원칙과 결합).
- `static int CalculateActualProductionQuantity(int shortageQuantity, double yield)`
  → `ceil(shortageQuantity / yield)` (PRD 4.6.1, 7절)
- `static Model::ProductionQueueItem::MinutesDuration CalculateTotalProductionTime(double averageProductionMinutesPerUnit, int actualProductionQuantity)`
  → `averageProductionMinutesPerUnit * actualProductionQuantity`
- `static Model::ProductionQueueItem CreateProductionQueueItem(const Model::Order& order, const Model::Sample& sample, int shortageQuantity, Model::Order::TimePoint approvedAt)`
  → 위 두 함수를 조합해 `ProductionQueueItem`을 생성한다(`productionStartTime`은 승인 시각을 그대로 사용;
  실제 FIFO 대기로 인한 시작 지연 반영은 Phase 6의 몫이며, 이 Phase는 큐 "등록"까지만 책임진다).
- 매직 넘버 없이 위 계산은 모두 이름이 붙은 함수로 분리해 테스트 가능성을 높인다(unit-tester가 수율 1.0 초과
  안 되는 경계, 정확히 나누어떨어지는 경우/나머지가 남는 경우의 `ceil` 처리를 검증할 수 있도록).

### 5. 시료가 삭제된 경우에 대한 방어적 처리
현재 `SampleController`는 시료 삭제(`Remove`) 기능을 노출하지 않으므로 "주문 접수 시점에 존재를 확인한
시료가 승인 시점에 사라지는" 상황은 현재 시스템 흐름상 발생하지 않는다. 그러나 향후 시료 삭제 기능이 추가될
가능성에 대비해 `ApproveOrder`가 `sampleController_.FindSampleById`에서 값을 얻지 못하면 예외를 던지거나
크래시하지 않고 `OrderApprovalResult::SampleNotFound`를 반환하도록 방어적으로 처리한다(주문 상태는 변경하지
않음). 이는 과설계가 아니라 "예상 밖 입력에 대해 안전하게 실패"하는 최소한의 방어 코드다.

### 6. 상태 전이 검증은 `Model::Order::TryTransitionTo`에 위임(중복 구현 금지)
- `ApproveOrder`는 먼저 주문 상태가 `RESERVED`인지 확인한 뒤(아니면 `OrderApprovalResult::InvalidOrderState`
  반환), `TryTransitionTo(Confirmed)` 또는 `TryTransitionTo(Producing)`을 호출한다. `Model::OrderStatus.cpp`의
  전이 테이블(`RESERVED → CONFIRMED`, `RESERVED → PRODUCING` 모두 허용됨)을 그대로 재사용하므로 Controller가
  전이 허용 여부를 별도로 재구현하지 않는다.
- `RejectOrder`도 동일하게 `order.TryTransitionTo(Rejected)`의 반환값(`bool`)을 그대로 `Success`/
  `InvalidOrderState`로 매핑한다. `REJECTED`가 이미 종단 상태이므로 `REJECTED`/`RELEASED`/`CONFIRMED`/
  `PRODUCING` 상태의 주문에 대해 `RejectOrder`를 호출하면 `TryTransitionTo`가 false를 반환해 자연스럽게
  차단된다(별도의 "이미 최종 상태인지" 분기를 추가하지 않음 — 이미 `OrderStatus.cpp`가 책임짐).

### 7. `GetReservedOrders()` 제공 이유
PRD 4.4.1 "접수된 주문 목록 — RESERVED 상태의 주문만 목록으로 표시"를 위해 `OrderApprovalController`가
`orderRepository_.GetAll()`을 필터링한 목록을 제공한다. 승인/거절 시 이 목록에 없는 주문번호를 입력해도
`ApproveOrder`/`RejectOrder`는 각각 저장소 조회 결과 자체로 유효성을 재검증하므로(View가 목록에 없는 값을
어떻게든 넘기더라도) 안전하다.

### 8. `main.cpp` 임시 배선 (Phase 9 이전 상태)
Phase 4와 동일하게 `MainMenuController`(Phase 9)가 아직 없으므로 상위 메뉴에 "[3] 주문 승인/거절"을 추가한다.
DI 조립 순서: `JsonSampleRepository` → `SampleController` → `JsonOrderRepository` → `OrderController`
(접수용) → `JsonProductionQueueRepository` → `OrderApprovalController`(orderRepository, sampleController,
productionQueueRepository) → `OrderApprovalView`. `OrderApprovalController`도 `SampleController`가 먼저
생성되어야 하는 의존 순서를 갖는다.

## 완료 조건(DoD) 매핑
- 재고 충분/부족 각 케이스에 대한 상태 전이 단위 테스트 통과: `ApproveOrder`가 `shortageQuantity <= 0`이면
  `CONFIRMED`, `> 0`이면 `PRODUCING` + 큐 등록으로 분기하도록 구현.
- 승인 시점에 재고가 물리적으로 차감되지 않음을 검증하는 테스트 존재: `ApproveOrder`가 `Sample::SetStock`을
  전혀 호출하지 않으므로, 승인 전후 `sampleRepository_.FindById(...)->GetStock()`이 동일함을 테스트로 확인
  가능(unit-tester 몫).
- `REJECTED` 이후 추가 전이가 시도되면 실패(또는 차단)함을 확인하는 테스트 존재: `RejectOrder`/`ApproveOrder`
  모두 `Model::Order::TryTransitionTo`에 위임하므로 `REJECTED` 상태의 주문에 대한 재호출은 `false`를 반환하고
  `OrderRejectionResult::InvalidOrderState`/`OrderApprovalResult::InvalidOrderState`로 매핑됨.
- 동일 시료에 대한 여러 승인 건이 각각 독립된 생산 큐 항목으로 생성됨을 확인하는 테스트 존재(병합되지 않음):
  서로 다른 주문번호로 승인하면 `IProductionQueueRepository::Create`가 각각 별도 항목으로 추가됨(주문번호가
  키이므로 병합 불가능하도록 Phase 2에서 이미 보장).

## 빌드 반영
- `SampleOrderSystem.vcxproj`/`.vcxproj.filters`에 신규 소스 4개(`.h` 2 + `.cpp` 2: OrderApprovalController,
  OrderApprovalView) 추가.
- `src/main.cpp`의 Release 분기 수정(`JsonProductionQueueRepository`/`OrderApprovalController`/
  `OrderApprovalView` 조립 및 상위 메뉴 항목 추가).
- Debug(x64) 빌드 및 실행 결과: 기존 테스트(Phase 0~4) 201개 전부 통과, 회귀 없음 확인
  (`SampleController::FindSampleById` 추가와 `OrderApprovalController`/`OrderApprovalView` 신설이 기존 동작에
  영향을 주지 않음).
- Release(x64) 빌드도 정상 성공(수동 실행 배선 확인용).
- 이번 Phase에서는 `Controller::OrderApprovalController`/`Controller::SampleController::FindSampleById`에 대한
  단위 테스트를 직접 작성하지 않았다(단위 테스트 작성은 unit-tester의 책임). unit-tester가 검증해야 할 대상은
  "다음으로 unit-tester가 검증할 대상" 절 참고.

## 다음으로 unit-tester가 검증할 대상
- `Controller::SampleController::FindSampleById`: 등록된 시료/미등록 시료 각각에 대한 반환값(옵셔널 값 유무,
  재고 값 일치).
- `Controller::OrderApprovalController::GetReservedOrders`: RESERVED 외 상태(REJECTED/PRODUCING/CONFIRMED/
  RELEASED)가 섞여 있을 때 RESERVED만 필터링되는지.
- `Controller::OrderApprovalController::ApproveOrder`
  - 재고가 주문 수량 이상이면 `Success` + 주문 상태 `CONFIRMED` 전환, 생산 큐에 항목이 생성되지 않음.
  - 재고가 주문 수량보다 부족하면 `Success` + 주문 상태 `PRODUCING` 전환, 생산 큐에 정확히 1건 등록되고
    `shortageQuantity`/`actualProductionQuantity`(`ceil(shortage/yield)`, 나누어떨어지는 경우와 나머지가
    남는 경우 모두)/`totalProductionTime`(평균 생산시간 × 실 생산량) 값이 기대대로 계산되는지.
  - 재고가 정확히 부족분 0(주문 수량 == 재고)인 경계 케이스에서 `CONFIRMED`로 전환되는지(부족분 0 → 재고
    충분 취급).
  - 승인 전후로 `sampleRepository`에 저장된 시료의 재고(`GetStock()`)가 전혀 변하지 않는지(물리적 차감 없음).
  - 존재하지 않는 주문번호로 승인 시 `OrderNotFound`, 저장소가 변경되지 않는지.
  - 이미 `RESERVED`가 아닌 주문(`REJECTED`/`PRODUCING`/`CONFIRMED`/`RELEASED`)에 대해 승인을 시도하면
    `InvalidOrderState`를 반환하고 상태가 바뀌지 않는지.
  - 동일 시료를 참조하는 서로 다른 두 주문을 모두 승인(둘 다 재고 부족)했을 때 생산 큐에 항목이 2건 생성되고
    병합되지 않는지(각각 다른 `orderId`를 키로 가짐).
- `Controller::OrderApprovalController::RejectOrder`
  - `RESERVED` 주문을 거절하면 `Success` + 상태 `REJECTED` 전환.
  - 이미 `REJECTED`인 주문을 다시 거절하면 `InvalidOrderState`(전이 차단)이고 상태가 유지되는지.
  - `PRODUCING`/`CONFIRMED`/`RELEASED` 주문에 대한 거절도 모두 `InvalidOrderState`로 차단되는지(PRD 3절 상태
    전이표상 REJECTED로 갈 수 있는 경로는 RESERVED뿐).
  - 존재하지 않는 주문번호로 거절 시 `OrderNotFound`.

## 재작업 이력
(현재까지 없음)
