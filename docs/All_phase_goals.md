# 개발 단계별 목표 (All Phase Goals)

이 문서는 `docs/PRD.md`의 요구사항을 어떤 순서로 구현할지 정의한다. 각 단계는 이전 단계의 산출물에
의존하므로 순서를 건너뛰지 않는다. 단계별 작업은 `docs/git_rules/COMMIT_PREVENTION.md` 규칙에 따라
의미 단위로 잘게 나누어 커밋한다.

각 Phase는 아래 형식으로 기술한다.
- **목표**: 이 단계에서 달성해야 하는 것
- **산출물**: 실제로 만들어지는 파일/구조
- **완료 조건(DoD)**: 다음 단계로 넘어가기 위한 최소 기준
- **관련 PRD**: `docs/PRD.md`의 해당 섹션

---

## Phase 0 — 프로젝트 셋업

**목표**: 빈 Visual Studio 스켈레톤에 실제 개발을 시작할 수 있는 기반을 마련한다.

**참고 저장소(PoC)**: 아래 4개 저장소는 별도로 개발된 PoC이며, 코드를 그대로 옮기지 않고 구조/방식만 참고한다(`CLAUDE.md` 저장소 범위 참고).
| PoC | 저장소 URL |
|---|---|
| MVC 스켈레톤 코드 | https://github.com/J2Yoon/ConsoleMVC-megacoffee-0715.git |
| 데이터 영속성 처리 | https://github.com/J2Yoon/DataPersistence-megacoffee-0715.git |
| 데이터 모니터링 Tool | https://github.com/J2Yoon/DataMonitor-megacoffee-0715.git |
| Dummy 데이터 생성 Tool | https://github.com/J2Yoon/DummyDataGenerator-megacoffee-0715.git |

**산출물**
- vcpkg 매니페스트(`vcpkg.json`) 구성, GoogleTest 의존성만 등록(JSON은 외부 라이브러리를 쓰지 않고 PoC와 동일하게 자체 구현하므로 vcpkg 대상이 아님)
- `CLAUDE.md`에 정의된 디렉터리 구조 생성: `src/Model`, `src/View`, `src/Controller`, `src/Persistence`, `src/Json`, `tests/`
- `.slnx`/`.vcxproj`에 테스트 프로젝트(`SampleOrderSystem.Tests`) 추가
- `main` 진입점만 있는 최소 빌드 가능 상태

**완료 조건(DoD)**
- `msbuild`로 빌드 성공
- 빈 GoogleTest 스위트가 실행되어 통과(0개 테스트라도 실행 자체가 성공)

**관련 PRD**: 1.3(시스템 형태), 6(비기능 요구사항 — 아키텍처/영속성)

---

## Phase 1 — 도메인 모델 정의

**목표**: View/Controller/저장 방식과 무관한 순수 도메인 모델을 먼저 확정한다.

**산출물**
- `Model/Sample`: 시료 ID, 이름, 평균 생산시간, 수율, 현재 재고
- `Model/Order`: 주문번호, 시료 ID, 고객명, 주문 수량, 상태(RESERVED/REJECTED/PRODUCING/CONFIRMED/RELEASED), 생성 일시
- `Model/ProductionQueueItem`: 연결 주문번호, 부족분, 실 생산량, 총 생산 시간, 생산 시작 시각, 예상 완료 시각
- 상태(enum) 및 상태 전이 유효성 검사 함수(잘못된 전이 방지)

**완료 조건(DoD)**
- 각 모델에 대한 생성/필드 접근 단위 테스트 통과
- 도메인 모델이 콘솔 입출력이나 파일 I/O에 의존하지 않음(컴파일 의존성 없음)

**관련 PRD**: 3(주문 상태 정의), 4.2.1(시료 속성), 5(데이터 모델 요약)

---

## Phase 2 — 데이터 영속성 (JSON 파일 + Repository)

**목표**: 모델을 JSON 파일로 저장/복원하는 Repository 계층을 구현하고 CRUD를 보장한다.

**참고 PoC**: `DataPersistence`(구조 패턴)와 `DummyDataGenerator`/`DataMonitor`(스키마 호환성 검증)를 참고한다. 상세 패턴은 `CLAUDE.md`의 "PoC별 참고 포인트" 표 참고.

**산출물**
- `src/Json/JsonValue.h/.cpp`(`Json::Value` 클래스), `src/Json/JsonIO.h/.cpp`(`Json::FileIO` 클래스): 외부 라이브러리 없이 PoC와 동일하게 자체 구현하는 JSON 값 표현/파서/직렬화기와 파일 read/write 유틸
- `Persistence/ISampleRepository`, `Persistence/IOrderRepository`, `Persistence/IProductionQueueRepository`: CRUD(Create/GetAll/FindById/Update/Remove) 순수 가상 인터페이스
- `Persistence/JsonSampleRepository`, `Persistence/JsonOrderRepository`, `Persistence/JsonProductionQueueRepository`: 위 인터페이스의 JSON 구현체. 생성자에서 `Load()` 후 CUD 시점마다 `Persist()`로 전체를 다시 기록(write-through)
- 모델 ↔ JSON 매핑(`ToJson`/`FromJson`)은 각 Repository 구현체 내부에 배치
- 파일 없음/파싱 실패 시 예외를 던지지 않고 빈 목록으로 안전하게 폴백하는 로직
- 저장 파일은 `data/samples.json`, `data/orders.json`이며 `docs/PRD.md` 5.4절의 PoC 호환 스키마(필드명 포함)를 그대로 따른다

**완료 조건(DoD)**
- 저장 → 재시작(프로세스 재기동) → 로드 시 데이터 동일함을 확인하는 단위/통합 테스트 통과
- CRUD(생성/조회/수정/삭제) 각각에 대한 테스트 존재
- 파일이 없을 때 예외 없이 빈 목록으로 기동함을 확인하는 테스트 존재
- (수동 검증) 옆 저장소의 `DummyDataGenerator.exe`로 이 프로젝트의 `data/` 폴더에 생성한 더미 데이터를 SampleOrderSystem이 오류 없이 읽어들이는지 확인

**관련 PRD**: 1.3, 5.4(JSON 파일 스키마), 6(데이터 영속성)

---

## Phase 3 — 시료 관리 기능

**목표**: 콘솔에서 시료를 등록/조회/검색할 수 있게 한다(MVC 세 계층 모두 연결하는 첫 기능).

**참고 PoC**: `ConsoleMVC`의 Controller/View 분리 및 명명 규칙(예: `SampleController`, `ConsoleView` 정적 I/O 헬퍼)을 참고한다.

**산출물**
- `Controller/SampleController`, `View/SampleView`
- 메뉴: 시료 등록 / 시료 조회(재고 포함) / 시료 검색(이름 기준)

**완료 조건(DoD)**
- 등록되지 않은 시료는 이후 단계(주문)에서 참조 불가함을 보장하는 검증 로직 포함
- 시료 등록·조회·검색 각각에 대한 컨트롤러 단위 테스트 통과

**관련 PRD**: 4.2(시료 관리)

---

## Phase 4 — 주문 접수(예약)

**목표**: 고객 주문을 `RESERVED` 상태로 시스템에 등록하는 기능을 구현한다.

**산출물**
- `Controller/OrderController`(예약 기능), `View/OrderView`
- 입력값(시료 ID, 고객명, 주문 수량) 검증: 미등록 시료 거부, 수량은 양수만 허용

**완료 조건(DoD)**
- 예약 성공 시 주문이 `RESERVED` 상태로 저장소에 반영됨을 테스트로 확인
- 잘못된 입력(미등록 시료 ID, 0 이하 수량)에 대한 거부 동작 테스트 통과

**관련 PRD**: 4.3(시료 주문)

---

## Phase 5 — 주문 승인/거절 (재고 판단 로직)

**목표**: `RESERVED` 주문에 대해 재고를 조회하여 자동으로 `CONFIRMED` 또는 `PRODUCING`으로 분기하거나, 거절 시 `REJECTED`로 전환한다.

**산출물**
- 재고 판단 로직: 부족분 = 주문 수량 − 현재 재고(조회만 하며 물리적 차감 없음)
- 재고 충분 → `CONFIRMED` 전환 / 재고 부족 → `PRODUCING` 전환 + 생산 큐 등록
- 거절 처리 → `REJECTED` 전환(종단 상태, 이후 전이 차단)
- 동일 시료에 대한 주문이라도 각각 별도의 생산 작업으로 처리(기존 생산 큐 항목과 병합하지 않음)

**완료 조건(DoD)**
- 재고 충분/부족 각 케이스에 대한 상태 전이 단위 테스트 통과
- 승인 시점에 재고가 물리적으로 차감되지 않음을 검증하는 테스트 존재
- `REJECTED` 이후 추가 전이가 시도되면 실패(또는 차단)함을 확인하는 테스트 존재
- 동일 시료에 대한 여러 승인 건이 각각 독립된 생산 큐 항목으로 생성됨을 확인하는 테스트 존재(병합되지 않음)

**관련 PRD**: 3(주문 상태 정의), 4.4(주문 승인/거절)

---

## Phase 6 — 생산 라인 (FIFO 큐 + 시간 기반 완료 처리)

**목표**: 단일 생산 라인에서 FIFO로 생산 큐를 처리하고, 실제 경과 시간에 따라 생산 완료 및 재고 반영을 수행한다.

**산출물**
- `Controller/ProductionLineController`: 실 생산량 `ceil(부족분/수율)`, 총 생산 시간 `평균 생산시간 × 실 생산량` 계산
- FIFO 큐 처리(선입선출, 큐 진입 후 취소·변경 불가)
- 생산 라인은 주문이 들어온 시료에 대해서만 동작(재고를 미리 예측 생산하지 않음)
- **시간 정산 로직**: 애플리케이션 재시작 시, 재시작 시점의 현재 시각을 기준으로 이미 완료됐어야 할 큐 항목을 일괄 정산(생산 완료 처리 + 재고 증가 + `PRODUCING → CONFIRMED` 전환)
- `View/ProductionLineView`: 현재 생산 중 정보, 대기 큐 목록 표시

**완료 조건(DoD)**
- 실 생산량/총 생산 시간 계산식에 대한 단위 테스트(수율에 따른 ceil 처리 포함) 통과
- 앱 재시작을 시뮬레이션했을 때(저장된 시작 시각 기준 경과 시간 계산) 완료됐어야 할 큐 항목이 올바르게 정산되는 테스트 통과
- 생산 완료 시 재고가 실 생산량만큼 증가하고 주문이 `CONFIRMED`로 전환됨을 확인하는 테스트 통과

**관련 PRD**: 4.6(생산 라인) — 특히 4.6.1, 4.6.4(실시간성/영속성)

---

## Phase 7 — 출고 처리

**목표**: `CONFIRMED` 주문에 대해 전량 출고를 처리한다.

**산출물**
- `Controller/ShipmentController`, `View/ShipmentView`
- 출고 가능 목록은 `CONFIRMED` 상태로 제한, 부분 출고 불가(항상 전체 수량)

**완료 조건(DoD)**
- 출고 처리 시 재고가 출고 수량만큼 감소하고 주문이 `RELEASED`로 전환되는 테스트 통과
- `CONFIRMED`가 아닌 주문은 출고 목록/처리 대상에서 제외됨을 확인하는 테스트 통과

**관련 PRD**: 4.7(출고 처리)

---

## Phase 8 — 모니터링

**목표**: 상태별 주문 현황과 시료별 재고 현황(여유/부족/고갈)을 확인할 수 있게 한다.

**참고 PoC**: `DataMonitor`의 `MonitoringService`처럼 집계 로직을 Repository/View와 분리한 순수 계산 서비스로 두는 구조와 `REJECTED` 제외 처리를 참고한다. 단, `DataMonitor`는 별도 실행파일(.exe)로 분리된 읽기 전용 프로세스이지만, 본 프로젝트의 모니터링은 **같은 프로세스 내 메뉴 기능**으로 통합한다(별도 exe로 분리하지 않는다).

**산출물**
- `Controller/MonitoringController`, `View/MonitoringView`
- 집계 전담 순수 계산 서비스(예: `MonitoringService`, 파일 접근 없이 이미 로드된 데이터만으로 계산)
- 상태별(RESERVED/CONFIRMED/PRODUCING/RELEASED) 집계, `REJECTED` 제외
- 재고 상태 판정(여유/부족/고갈) 로직

**완료 조건(DoD)**
- 각 집계·판정 로직에 대한 단위 테스트 통과
- `REJECTED` 주문이 집계에서 실제로 빠짐을 확인하는 테스트 존재
- (수동 검증, 선택) 옆 저장소의 `DataMonitor.exe`를 이 프로젝트의 `data/` 폴더로 실행했을 때도 동일한 집계 결과가 나오는지 교차 확인

**관련 PRD**: 4.5(모니터링)

---

## Phase 9 — 메인 메뉴 통합

**목표**: 지금까지 만든 개별 기능(시료 관리/주문/승인·거절/생산 라인/출고/모니터링)을 하나의 메인 메뉴로 연결한다.

**산출물**
- `Controller/MainMenuController`(또는 앱 진입점): 메뉴 선택 → 각 하위 컨트롤러 라우팅
- 앱 시작 시 요약 정보(등록 시료 수, 총 재고, 전체 주문 수, 생산라인 대기 건수) 표시
- 앱 시작 시 Phase 6의 생산 큐 시간 정산 로직을 먼저 수행

**완료 조건(DoD)**
- 전체 시나리오(시료 등록 → 주문 접수 → 승인(재고부족) → 생산 대기/완료 → 출고 → 모니터링 확인)를 콘솔에서 처음부터 끝까지 수동으로 검증 가능
- 종료 후 재시작해도 데이터와 생산 진행 상태가 올바르게 이어짐을 수동 검증
- (선택) 옆 저장소의 `DummyDataGenerator.exe`로 이 프로젝트의 `data/` 폴더에 대량의 시료/주문 더미 데이터를 생성해 수동 시나리오 검증에 활용 가능

**관련 PRD**: 4.1(메인 메뉴)

---

## Phase 10 — 테스트 보강 및 Clean Code 정리

**목표**: 기능 구현 과정에서 누락된 테스트를 보강하고, 코드 전반의 가독성/일관성을 점검한다.

**산출물**
- 계층별(Model/Controller/Persistence) 단위 테스트 커버리지 점검 및 누락분 보강
- 중복 로직 제거, 네이밍/구조 일관성 정리(`CLAUDE.md` 코딩 컨벤션 기준)
- 불필요한 주석/죽은 코드 제거
- `CLAUDE.md` "Clean Code 원칙" 절 기준 점검: 책임이 뒤섞인 긴 함수 분리, 의미 불명확한 변수/함수명 정리,
  매직 넘버/문자열을 상수·enum으로 치환, 상태 전이/재고 판정/생성 로직 등 목적에 맞는 지점에 GoF 패턴이
  과설계 없이 적용되어 있는지 재검토

**완료 조건(DoD)**
- 전체 테스트 스위트 통과
- `CLAUDE.md`의 핵심 도메인 규칙 체크리스트를 다시 훑어 위반 사항 없음을 확인
- `CLAUDE.md` Clean Code 원칙(함수화, 네이밍, 매직 넘버 금지, 목적에 맞는 디자인 패턴 적용) 위반 사항 없음을 확인

**관련 PRD**: 6(Test, CleanCode)

---

## Phase 11 — 최종 점검

**목표**: 제출 전 문서/이력/동작을 최종 확인한다.

**산출물**
- `docs/PRD.md`, `CLAUDE.md`, `docs/All_phase_goals.md` 최신화 여부 확인
- 커밋 이력이 `docs/git_rules/COMMIT_PREVENTION.md` 규칙(의미 단위, 메시지 형식)을 따르는지 재검토
- 전체 기능 시나리오 최종 리허설(수동 시연)

**완료 조건(DoD)**
- 신규/불일치 사항 없음
- 저장소가 Public 상태이며 제출 요건(Repository 이름 규칙 등)을 충족

**관련 PRD**: 전체
