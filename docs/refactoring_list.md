# 리팩터링 목록 (Refactoring List)

`refactoring-agent`가 3개 Phase가 끝날 때마다(Phase 0/1/2, Phase 3/4/5, ...) 실행되며 이 문서에
리팩터링 후보를 기록하고, 실제로 처리한 항목은 `[x]`로 표시한다. 형식은
`.claude/agents/refactoring-agent.md`의 "문서 형식" 절을 따른다.

- `[ ]`: 미완료(다음 실행 때 이어서 처리)
- `[x]`: 완료

---

## [x] Repository 3종 간 CRUD 로직 중복 (DRY 위반)
- 발견 시점: Phase 2 종료 후 (phase-developer가 구현 중 자체 발견)
- 대상 파일: `src/Persistence/JsonSampleRepository.cpp`, `src/Persistence/JsonOrderRepository.cpp`,
  `src/Persistence/JsonProductionQueueRepository.cpp`
- 문제: `JsonSampleRepository`, `JsonOrderRepository`, `JsonProductionQueueRepository`가
  `Load`/`Persist`/`FindById`/`Update`/`Remove`를 사실상 동일한 구조로 각각 재작성하고 있다.
  `std::find_if`로 키를 찾아 조회/수정/삭제하는 부분, `Load`에서 파일 없음/파싱 실패 시 빈 목록으로
  폴백하는 부분, `Persist`에서 배열 전체를 다시 덤프하는 부분이 세 클래스에 걸쳐 글자 그대로 반복된다.
  Repository가 하나 더 늘어나면 동일 로직을 네 곳에서 고쳐야 한다.
- 조치: 새 헤더 `src/Persistence/JsonRepositoryUtil.h`에 `LoadEntitiesFromFile`/`PersistEntitiesToFile`/
  `FindIteratorByKey` 템플릿 함수(Template Method 방식)를 추출했다. 각 Repository는 엔티티별
  `ToJson`/`FromJson`(기존 그대로 유지)과 키 추출 함수(`GetSampleKey`/`GetOrderKey`/
  `GetProductionQueueItemKey`, 각 .cpp의 익명 네임스페이스에 배치)만 제공하고, `Load`/`Persist`/
  `FindById`/`Update`/`Remove`의 몸체는 공용 유틸리티 호출로 대체했다. 클래스의 public 인터페이스,
  파일 스키마, write-through/폴백 정책은 전혀 변경하지 않았다(순수 구현 세부사항 리팩터링).
  `.vcxproj`/`.vcxproj.filters`에 새 헤더를 `ClInclude`로 추가했다.
- 완료 시점: Phase 0~2 종료 후 리팩터링 에이전트 실행 (Debug/Release 빌드 및 GoogleTest 162개 전체 통과 확인)

## [x] OrderStatus 문자열 변환 스타일 불일치 (사소함)
- 발견 시점: Phase 2 종료 후 (phase-developer가 구현 중 자체 발견)
- 대상 파일: `src/Model/OrderStatus.cpp`, `src/Persistence/JsonOrderRepository.cpp`
- 문제: `OrderStatus.cpp`는 전이 테이블을 `constexpr std::array`로 선언형 관리하는데,
  `JsonOrderRepository::OrderStatusToString`/`OrderStatusFromString`은 `switch`/`if` 체인으로 되어 있어
  같은 저장소 안에서 스타일이 갈린다.
- 조치: `JsonOrderRepository.cpp`의 익명 네임스페이스에 `OrderStatus.cpp`와 동일한 스타일의
  `constexpr std::array<std::pair<Model::OrderStatus, const char*>, 5> kOrderStatusNames` 테이블을
  추가하고, `OrderStatusToString`/`OrderStatusFromString`이 이 테이블을 순회하도록 바꿨다. 알 수 없는
  값에 대한 예외 던짐(`ToString`)/`Reserved` 폴백(`FromString`) 동작은 그대로 유지했다.
- 완료 시점: Phase 0~2 종료 후 리팩터링 에이전트 실행 (Debug/Release 빌드 및 GoogleTest 162개 전체 통과 확인)

## [x] Controller 계층 패턴 적용 (미착수 — Controller 구현 이후 재검토)
- 발견 시점: Phase 2 종료 후 (phase-developer가 구현 중 자체 발견)
- 대상 파일: `src/Controller/` (아직 비어 있음)
- 문제: CLAUDE.md에서 언급한 State/Strategy/Factory Method 패턴(주문 상태 전이, 재고 판정,
  ProductionQueueItem 생성)은 Controller 계층 로직에 적용될 대상인데, 아직 `src/Controller/`가 비어
  있어 해당 코드 자체가 없다.
- 조치: Phase 3~5(SampleController/OrderController/OrderApprovalController)에서 실제로 검토한 결과,
  상태 전이는 이미 `Model::IsValidOrderStatusTransition` 전이 테이블 + `Order::TryTransitionTo`로,
  `ProductionQueueItem` 생성은 `OrderApprovalController::CreateProductionQueueItem`(Factory Method
  스타일 정적 팩토리 함수)로 이미 구현 시점에 적용되어 있음을 확인했다. 재고 판정(여유/부족)은 분기가
  2갈래(`shortageQuantity <= 0` 여부)뿐이라 Strategy 패턴을 적용하면 오히려 과설계이므로 적용하지
  않기로 결정했다(CLAUDE.md "패턴은 목적 없이 남용하지 않는다" 원칙). 추가 코드 변경 없이 현황 확인만
  수행하고 완료로 표시한다.
- 완료 시점: Phase 3~5 종료 후 리팩터링 에이전트 실행 (Debug 빌드 및 GoogleTest 242개 전체 통과 확인)

## [x] OrderStatus <-> 문자열 변환 로직이 Persistence/View 두 곳에서 각각 재구현됨
- 발견 시점: Phase 3~5 종료 후 (Phase 4 OrderView 구현 시 재도입)
- 대상 파일: `src/Model/OrderStatus.h`, `src/Model/OrderStatus.cpp`,
  `src/Persistence/JsonOrderRepository.h`, `src/Persistence/JsonOrderRepository.cpp`,
  `src/View/OrderView.h`, `src/View/OrderView.cpp`
- 문제: Phase 0~2 리팩터링에서 `JsonOrderRepository`의 문자열 변환을 `Model::OrderStatus.cpp`와
  같은 선언형 테이블 스타일로 통일했지만, 두 매핑 테이블 자체는 여전히 각 파일에 따로 존재했다.
  이후 Phase 4에서 `OrderView::OrderStatusToDisplayText`가 동일한 `RESERVED`/`REJECTED`/... 매핑을
  `switch`문으로 세 번째로 재구현하면서 같은 지식(OrderStatus의 대문자 문자열 표기)이 세 곳(Persistence
  private 테이블, View private switch)에 흩어졌다. 상태값이 추가/변경되면 세 곳을 모두 고쳐야 하는
  구조였다.
- 조치: `Model::OrderStatus.h/.cpp`에 `OrderStatusToString`/`OrderStatusFromString`을 공개 자유
  함수로 승격하고, 기존 `kOrderStatusNames` 테이블을 이 파일로 이전했다. `JsonOrderRepository`의
  private static `OrderStatusToString`/`OrderStatusFromString`(및 로컬 테이블)을 제거하고
  `ToJson`/`FromJson`에서 `Model::OrderStatusToString`/`Model::OrderStatusFromString`을 직접
  호출하도록 바꿨다. `View::OrderView`의 `OrderStatusToDisplayText` switch문도 제거했다.
  파일 스키마(대문자 문자열 값), `Update`/`FromString`의 알 수 없는 값 폴백 정책(Reserved로 폴백)은
  그대로 유지했다.
- 완료 시점: Phase 3~5 종료 후 리팩터링 에이전트 실행 (Debug 빌드 및 GoogleTest 242개 전체 통과 확인)

## [x] OrderView/OrderApprovalView 간 주문 표 출력 로직 중복
- 발견 시점: Phase 3~5 종료 후 (Phase 4/5에서 각각 독립적으로 구현되며 재도입)
- 대상 파일: `src/View/OrderView.h`, `src/View/OrderView.cpp`,
  `src/View/OrderApprovalView.h`, `src/View/OrderApprovalView.cpp`, `src/View/ConsoleView.h`,
  `src/View/ConsoleView.cpp`
- 문제: `OrderView::PrintOrderTableHeader`/`PrintOrderRow`와 `OrderApprovalView::PrintOrderTableHeader`/
  `PrintOrderRow`가 열 너비(`setw(20/10/16/8)`)까지 완전히 동일한 코드를 각 View에 따로 두고 있었다.
  `OrderApprovalView::PrintOrderRow`는 한 술 더 떠 상태 문자열을 `"RESERVED"` 매직 문자열로
  하드코딩하고 있어, 상태 표시 형식이 바뀌면 두 View를 모두 고쳐야 하고 하드코딩된 값과도 실제로
  일치하는지 보장되지 않았다(우연히 이 화면이 RESERVED 주문만 표시해서 지금까지는 문제가 없었을 뿐).
- 조치: 공용 콘솔 I/O 헬퍼인 `View::ConsoleView`에 `PrintOrderTableHeader()`/
  `PrintOrderRow(const Model::Order&)`를 추가(내부적으로 `Model::OrderStatusToString` 사용)하고,
  `OrderView`/`OrderApprovalView`는 각자의 private static 중복 메서드를 제거한 뒤
  `ConsoleView::PrintOrderTableHeader()`/`ConsoleView::PrintOrderRow(order)`를 호출하도록 바꿨다.
  이미 CLAUDE.md 아키텍처 절에 명시된 "`ConsoleView` 공용 I/O 헬퍼" 역할을 그대로 확장한 것으로,
  새 클래스/인터페이스를 추가하지 않았다(과설계 없이 기존 구조 재사용). 두 화면의 출력 형식(열 너비,
  표시 문구)은 리팩터링 전후로 동일하다.
- 완료 시점: Phase 3~5 종료 후 리팩터링 에이전트 실행 (Debug 빌드 및 GoogleTest 242개 전체 통과 확인)
