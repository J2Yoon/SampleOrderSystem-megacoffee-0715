# 리팩터링 목록 (Refactoring List)

`refactoring-agent`가 3개 Phase가 끝날 때마다(Phase 0/1/2, Phase 3/4/5, ...) 실행되며 이 문서에
리팩터링 후보를 기록하고, 실제로 처리한 항목은 `[x]`로 표시한다. 형식은
`.claude/agents/refactoring-agent.md`의 "문서 형식" 절을 따른다.

- `[ ]`: 미완료(다음 실행 때 이어서 처리)
- `[x]`: 완료

---

## [ ] Repository 3종 간 CRUD 로직 중복 (DRY 위반)
- 발견 시점: Phase 2 종료 후 (phase-developer가 구현 중 자체 발견)
- 대상 파일: `src/Persistence/JsonSampleRepository.cpp:85-125`,
  `src/Persistence/JsonOrderRepository.cpp:121-161`,
  `src/Persistence/JsonProductionQueueRepository.cpp:100-140`
- 문제: `JsonSampleRepository`, `JsonOrderRepository`, `JsonProductionQueueRepository`가
  `Load`/`Persist`/`FindById`/`Update`/`Remove`를 사실상 동일한 구조로 각각 재작성하고 있다.
  `std::find_if`로 키를 찾아 조회/수정/삭제하는 부분, `Load`에서 파일 없음/파싱 실패 시 빈 목록으로
  폴백하는 부분, `Persist`에서 배열 전체를 다시 덤프하는 부분이 세 클래스에 걸쳐 글자 그대로 반복된다.
  Repository가 하나 더 늘어나면 동일 로직을 네 곳에서 고쳐야 한다.
- 조치: (미완료) `ToJson`/`FromJson`(엔티티별로 다름)과 키 추출자만 각 클래스가 제공하고, `Load`/
  `Persist`/`FindById`/`Update`/`Remove`의 공통 뼈대는 별도로 뽑아내는 것을 검토(Template Method류).
  단, CLAUDE.md가 패턴 남용을 금지하고 있고 현재는 Repository가 3개뿐이라 감내 가능한 수준의 중복이므로
  4번째 Repository가 추가되는 시점, 또는 refactoring-agent가 여유가 있을 때 처리하는 것으로 보류했다.
- 완료 시점: (비움)

## [ ] OrderStatus 문자열 변환 스타일 불일치 (사소함)
- 발견 시점: Phase 2 종료 후 (phase-developer가 구현 중 자체 발견)
- 대상 파일: `src/Model/OrderStatus.cpp:13-20`, `src/Persistence/JsonOrderRepository.cpp:48-69`
- 문제: `OrderStatus.cpp`는 전이 테이블을 `constexpr std::array`로 선언형 관리하는데,
  `JsonOrderRepository::OrderStatusToString`/`OrderStatusFromString`은 `switch`/`if` 체인으로 되어 있어
  같은 저장소 안에서 스타일이 갈린다.
- 조치: (미완료) 동일한 테이블 방식으로 통일하면 상태값 추가 시 실수 여지가 줄어든다. 다만 현재 5개
  상태 수준에서는 가독성 차이가 크지 않아 우선순위가 낮다고 보류했다.
- 완료 시점: (비움)

## [ ] Controller 계층 패턴 적용 (미착수 — Controller 구현 이후 재검토)
- 발견 시점: Phase 2 종료 후 (phase-developer가 구현 중 자체 발견)
- 대상 파일: `src/Controller/` (아직 비어 있음)
- 문제: CLAUDE.md에서 언급한 State/Strategy/Factory Method 패턴(주문 상태 전이, 재고 판정,
  ProductionQueueItem 생성)은 Controller 계층 로직에 적용될 대상인데, 아직 `src/Controller/`가 비어
  있어 해당 코드 자체가 없다.
- 조치: (미착수) Controller가 구현되는 Phase(3 이후) 이후 refactoring-agent 실행 시점에 다시 검토한다.
- 완료 시점: (비움)
