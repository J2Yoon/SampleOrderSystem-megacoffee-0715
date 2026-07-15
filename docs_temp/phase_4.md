# Phase 4 — 주문 접수(예약)

## 관련 문서
- `docs/All_phase_goals.md` Phase 4 (주문 접수(예약))
- `docs/PRD.md` 4.3(시료 주문 접수), 5.2(주문 데이터 모델 — 주문번호 포맷 예시 `ORD-YYYYMMDD-XXXX`)
- `CLAUDE.md` 아키텍처 절(Controller/View 계층 원칙), 핵심 도메인 규칙("주문 상태", "등록되지 않은 시료는
  주문할 수 없다")
- doc-verifier의 Phase 4 착수 전 검증 결과(GO), 참고 사항 2건 반영 대상:
  1. `OrderController`가 `Controller::SampleController::IsSampleRegistered`에 의존한다는 사실을 명시
  2. 주문번호 포맷(`ORD-YYYYMMDD-XXXX`) 채택 방식 결정·기록

## 목표
Phase 1의 `Model::Order`(RESERVED 전용 생성자, 상태 전이 검증)와 Phase 2의
`Persistence::IOrderRepository`/`JsonOrderRepository`, Phase 3의 `Controller::SampleController`를 연결해
"고객 주문을 RESERVED 상태로 접수"하는 첫 주문 기능(Controller+View)을 구현한다. 이 Phase는 승인/거절,
재고 판단, 생산 큐 등록을 다루지 않는다(Phase 5의 몫). 재고를 물리적으로 차감하지 않는다.

## 산출물 목록

| 파일 | 내용 | 완료 조건 매핑 |
|---|---|---|
| `src/Controller/OrderController.h/.cpp` | `Controller::OrderPlacementResult` enum(Success/UnregisteredSample/InvalidInput), `OrderController` 클래스: `PlaceOrder(sampleId, customerName, quantity, createdAt = now())`, `GetAllOrders()`. 생성자는 `Persistence::IOrderRepository&`와 `const Controller::SampleController&`를 주입받는다 | PRD 4.3.1, DoD "예약 성공 시 RESERVED로 저장", "잘못된 입력 거부" |
| `src/View/OrderView.h/.cpp` | 주문 접수 서브메뉴(주문 접수/전체 주문 목록 조회/뒤로) 표시 및 입력 위임. 도메인 로직 없이 `OrderController` 호출 결과만 표시 | CLAUDE.md "View는 도메인 로직을 갖지 않는다" |
| `src/main.cpp`(Release 분기) | `JsonOrderRepository` 추가, `SampleController` → `OrderController`(SampleController 참조 주입) → `OrderView` 순으로 조립. 메인 메뉴에서 "[1] 시료 관리" / "[2] 주문 접수"로 분기하는 임시 상위 루프 도입(Phase 9 이전 임시 배선) | 수동 실행으로 시료 등록 → 주문 접수 흐름 확인 가능 |

## 설계 결정

### 1. `OrderController`의 `SampleController` 의존 방식 (doc-verifier 참고사항 1)
- "등록되지 않은 시료는 주문할 수 없다"(PRD 4.2, CLAUDE.md 핵심 도메인 규칙)를 검증하기 위해
  `OrderController`는 생성자에서 `const Controller::SampleController& sampleController_`를 참조로 주입받아
  `sampleController_.IsSampleRegistered(sampleId)`를 그대로 호출한다.
- `ISampleRepository`를 직접 주입받아 `FindById`를 다시 호출하는 방식도 검토했으나, 그렇게 하면 "미등록
  시료 검증"이라는 동일한 책임이 `SampleController`와 `OrderController` 두 곳에 중복 구현된다. Phase 3에서
  이미 `SampleController::IsSampleRegistered`를 "이후 단계(주문 접수 등)에서 미등록 시료를 참조하지 못하도록
  검증할 때 사용"할 목적으로 공개해 두었으므로, `OrderController`는 이를 재사용하는 컨트롤러 간 협력
  (Controller-to-Controller collaboration)으로 구현한다. `SampleController`는 상태를 변경하지 않는 조회만
  하므로 `const&`로 주입해 `OrderController`가 시료 데이터를 변경할 수 없음을 타입으로 보장한다.
- `main.cpp`에서 DI 조립 순서는 `JsonSampleRepository` → `SampleController` → `JsonOrderRepository` →
  `OrderController(orderRepository, sampleController)` → `OrderView` 순이며, `SampleController`가
  `OrderController`보다 먼저 생성되어야 한다는 의존 순서를 `docs_temp/phase_4.md`(본 문서)와 `main.cpp` 주석에
  남긴다.

### 2. 주문번호 포맷 채택 방식 (doc-verifier 참고사항 2)
- PRD 5.2절 예시 `ORD-YYYYMMDD-XXXX`를 그대로 채택한다: `ORD-` 접두어 + 주문 접수 시각의 로컬 날짜
  (`YYYYMMDD`, 8자리) + `-` + 4자리 일별 순번(`0001`부터 시작, 앞자리 0 패딩).
- **순번 산정 방식**: 별도의 영속 시퀀스 카운터 파일을 새로 두지 않고, `IOrderRepository::GetAll()`로 조회한
  기존 주문 중 주문번호가 동일한 날짜 접두어(`ORD-YYYYMMDD-`)로 시작하는 것만 필터링해 접미 4자리를
  `std::stoi`로 파싱한 뒤 최댓값 + 1을 다음 순번으로 사용한다. 이는 `CLAUDE.md` "PoC별 참고 포인트" 표의
  `DummyDataGenerator` 행에서 언급된 "ID 시퀀스를 기존 데이터에서 파싱해 이어서 생성(추가 전용)" 아이디어를
  참고한 것이다(코드를 복사하지 않고 동일한 설계 아이디어만 재구현).
  - 장점: 이미 Phase 2에서 구현된 `IOrderRepository::GetAll()`만으로 순번을 결정할 수 있어 새로운 영속
    상태(별도 카운터 저장소)를 추가하지 않는다. 앱이 재시작되어도 `orders.json`에 이미 기록된 주문에서
    순번을 다시 계산하므로 별도의 정산 로직이 필요 없다.
  - 이 앱은 단일 프로세스·단일 스레드로 순차 실행되므로(콘솔 메뉴 루프), 같은 실행 안에서 동시에 두 주문이
    같은 순번을 받는 동시성 문제는 발생하지 않는다. 따라서 별도의 락/재시도 로직 없이 "생성 직전에 조회 →
    다음 순번 계산 → `Create` 호출"만으로 충분하다(과설계 방지).
  - 날짜 포맷은 `std::chrono::system_clock::to_time_t` + `localtime_s`(Windows) + `std::put_time("%Y%m%d")`로
    구현한다(PoC 저장소에 의존하지 않는 표준 라이브러리만 사용).
- **테스트 용이성**: `PlaceOrder`는 `createdAt` 파라미터를 `Model::Order::TimePoint` 타입으로 받되 기본값을
  `std::chrono::system_clock::now()`로 두어, 운영 시엔 인자 생략으로 현재 시각을 쓰고 단위 테스트에서는
  고정된 시각을 명시적으로 주입해 날짜 접두어/순번 계산을 결정론적으로 검증할 수 있게 한다.

### 3. 입력 검증 책임 소재: Controller 경계에서 수행 (Phase 3와 동일 원칙)
- CLAUDE.md 코딩 컨벤션: "예외적인 입력(음수 수량, 미등록 시료 ID 등)은 시스템 경계(사용자 입력 처리 계층)
  에서만 검증한다." Phase 3와 동일하게, 콘솔 입력 파싱은 `ConsoleView`가 담당하고 도메인 유효성 판정은
  `OrderController`가 담당한다.
- 검증 순서: (1) `sampleController_.IsSampleRegistered(sampleId)`가 false면 `UnregisteredSample` 반환,
  (2) 수량이 0 이하이거나 고객명이 비어있으면 `InvalidInput` 반환. 두 결과를 분리한 이유는 View가 "등록되지
  않은 시료입니다"와 "수량/고객명을 확인해주세요"를 서로 다른 안내 문구로 보여줄 수 있게 하기 위함이며,
  Phase 3의 `SampleRegistrationResult` 패턴을 그대로 재사용한다(매직 불리언 대신 명명된 결과 enum).
- 이 이상의 패턴(State/Strategy 등)은 이번 Phase의 분기가 단순 2~3갈래에 불과해 과설계이므로 도입하지 않는다.
  상태 전이 자체는 이미 `Model::OrderStatus`/`Order::TryTransitionTo`가 담당하며, 이번 Phase에서는 항상
  `RESERVED` 상태로만 생성하므로 전이 로직을 호출하지 않는다(신규 접수용 생성자가 이미 RESERVED로 고정).

### 4. `main.cpp` 임시 배선 (Phase 9 이전 상태)
- Phase 3와 마찬가지로 `MainMenuController`(Phase 9)가 아직 없으므로, Release 진입점에 최소한의 상위 메뉴
  루프("[1] 시료 관리 [2] 주문 접수 [0] 종료")를 추가해 `SampleView`와 `OrderView`를 모두 수동으로 검증할 수
  있게 한다. 이 상위 루프는 Phase 9에서 `MainMenuController`로 완전히 대체될 예정이므로 로직을 최소화한다.

## 완료 조건(DoD) 매핑
- 예약 성공 시 주문이 `RESERVED` 상태로 저장소에 반영됨: `OrderController::PlaceOrder`가 성공 시
  `Model::Order`의 신규 접수용 생성자(상태 RESERVED 고정)로 생성한 뒤 `IOrderRepository::Create`를 호출.
- 잘못된 입력(미등록 시료 ID, 0 이하 수량)에 대한 거부 동작: `UnregisteredSample`/`InvalidInput` 반환,
  저장소에 `Create`를 호출하지 않음.

## 빌드 반영
- `SampleOrderSystem.vcxproj`/`.vcxproj.filters`에 신규 소스 4개(`.h` 2 + `.cpp` 2: OrderController, OrderView)
  추가.
- `src/main.cpp`의 Release 분기 수정(`JsonOrderRepository`/`OrderController`/`OrderView` 조립 및 상위 메뉴 루프
  추가).
- Debug(x64) 빌드로 기존 테스트(Phase 0~3, 185개) 회귀 없음 확인.

## 재작업 이력
(현재까지 없음)
