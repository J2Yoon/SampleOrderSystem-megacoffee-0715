# CLAUDE.md

이 파일은 Claude Code가 이 저장소에서 작업할 때 참고하는 프로젝트 가이드입니다.
기능 요구사항과 도메인 규칙의 원본은 항상 **[docs/PRD.md](docs/PRD.md)** 이며, 이 문서와 PRD가 상충하면 PRD를 따른다.

## 프로젝트 개요

"반도체 시료 생산주문관리 시스템" — 반도체 시료(Sample)의 등록, 주문 접수/승인/거절, 생산, 출고 전 과정을
콘솔에서 관리하는 애플리케이션. 상세 기능 명세, 상태 전이, 계산식은 `docs/PRD.md` 참고.

## 저장소 범위 (중요)

- 미션에서 요구하는 4개의 PoC(MVC 스켈레톤 코드, 데이터 영속성 처리, 데이터 모니터링 Tool, Dummy 데이터 생성 Tool)는
  **이 저장소에서 개발하지 않는다.** 각각 별도의 독립 Repository(예: `ConsoleMVC-...`, `DataPersistence-...`,
  `DataMonitor-...`, `DummyDataGenerator-...`)에서 개발된다.
- 이 저장소(`SampleOrderSystem`)는 **본 프로젝트(반도체 시료 생산주문관리 시스템)만** 구현한다.
- PoC 저장소의 코드/구조는 참고 자료일 뿐이며, 그대로 복사해오지 않고 본 프로젝트 구조(아래 아키텍처 절)에 맞게 구현한다.
- **PoC 저장소를 라이브러리/패키지/서브모듈로 참조하지 않는다.** vcpkg 의존성, `#include` 경로, 프로젝트 참조
  (project reference), NuGet/DLL 등 어떤 형태로도 PoC 저장소의 산출물을 이 저장소에 연결하거나 가져오지 않는다.
  PoC의 `Json::Value`/`Json::FileIO`, Repository 패턴 등은 **개발 시점에 코드를 읽고 동일한 설계를 이 저장소
  안에 처음부터 새로 작성**하는 방식으로만 활용한다(설계·네이밍·동작 방식을 유사하게 재구현하되, 실행 시점에
  PoC 저장소의 코드나 바이너리에 의존하지 않는다). 단, `data/` 폴더의 JSON 파일 스키마를 호환되게 맞춤으로써
  옆 저장소의 `DummyDataGenerator.exe`/`DataMonitor.exe` **실행 파일을 데이터 파일을 매개로 나란히 실행**하는
  것은 가능하다 — 이는 코드/라이브러리 의존이 아니라 파일 포맷 호환일 뿐이다.
- 실제로 개발에 참고할 4개 PoC 저장소는 다음과 같다.

| PoC | 저장소 URL |
|---|---|
| MVC 스켈레톤 코드 | https://github.com/J2Yoon/ConsoleMVC-megacoffee-0715.git |
| 데이터 영속성 처리 | https://github.com/J2Yoon/DataPersistence-megacoffee-0715.git |
| 데이터 모니터링 Tool | https://github.com/J2Yoon/DataMonitor-megacoffee-0715.git |
| Dummy 데이터 생성 Tool | https://github.com/J2Yoon/DummyDataGenerator-megacoffee-0715.git |

### PoC별 참고 포인트 (구조 분석 결과 반영)

4개 PoC는 로컬에서 `../ConsoleMVC`, `../DataPersistence`, `../DataMonitor`, `../DummyDataGenerator`로 확인 가능하며,
공통적으로 `Models/`(순수 struct, 로직 없음) → `Repositories/`(`I{Entity}Repository` 인터페이스 + 구현체) →
`Controllers`/`Services` → `Views`(콘솔 I/O 전담) 계층 구조와 아래 네이밍 컨벤션을 공유한다. **이 컨벤션을
SampleOrderSystem에도 동일하게 적용한다.**

- 클래스: `PascalCase`, 계층 접미사 포함(`OrderController`, `IOrderRepository`, `JsonOrderRepository`).
- 인터페이스: `I` 접두사(`ISampleRepository`).
- 메서드: `PascalCase` 동사형(`PlaceOrder`, `ApproveOrder`, `FindById`).
- private 멤버 변수: `camelCase` + 트레일링 언더스코어(`orderRepository_`, `samples_`).
- 파일명 = 클래스명, `.h`/`.cpp` 쌍으로 분리.
- `main.cpp`에서 Repository 구체 클래스 → Controller(생성자 참조 주입) → View 순으로 수동 DI 조립(프레임워크 없음).
- View는 Controller만 참조하고 Controller는 `<iostream>`을 모른다(콘솔 I/O는 View에만 존재).

각 PoC에서 그대로 따를 패턴과 참고만 할 부분:

| PoC | 그대로 따를 패턴 | 참고만 하고 새로 구현할 부분 |
|---|---|---|
| ConsoleMVC | Model/Repository/Controller/View 4계층 분리, `main.cpp` 수동 DI 조립, 메뉴 루프+switch 구조, `ConsoleView` 공용 I/O 헬퍼 분리 | `Sample`/`Order`/`ProductionJob` 필드와 Controller 분해 방식은 이 PoC 특유의 것이므로 PRD 기준으로 재설계. 생산 라인의 "실시간 경과 미반영(수동 완료 트리거)"은 PoC 한계이며 본 프로젝트에서는 반드시 실제 경과시간 기반으로 구현 |
| DataPersistence | `I{Entity}Repository`(CRUD) + `Json{Entity}Repository` 구현 분리, 생성자에서 `Load()` 후 CUD마다 `Persist()`로 전체 재기록(write-through), 파일 없음/파싱 실패 시 예외 없이 빈 목록으로 폴백, `ToJson`/`FromJson`을 Repository의 private static 메서드로 배치 | Json/Repositories가 최상위 폴더에 있는 배치는 그대로 따르지 않고 `src/Persistence/`로 모은다. 무음(silent) 폴백 정책은 유지하되 필요 시 로깅 추가 고려 |
| DataMonitor | 집계 로직을 Reader/Repository와 분리한 순수 계산 서비스(`MonitoringService`, static 메서드, 파일 접근 없음)로 두는 구조, `REJECTED` 상태를 집계에서 제외하는 처리 | 이 PoC는 **읽기 전용 별도 프로세스(별도 .exe)** 이지만, 본 프로젝트의 모니터링은 같은 프로세스 내 메뉴 기능으로 통합한다(별도 실행파일로 분리하지 않음). `Sleep`+`system("cls")` 워치 모드는 참고하지 않음 |
| DummyDataGenerator | ID 시퀀스를 기존 데이터에서 파싱해 이어서 생성(추가 전용) 방식, `std::mt19937`+`discrete_distribution` 기반 가중치 랜덤 생성 아이디어 | 하드코딩된 소재/고객사 목록과 수치 범위는 예시일 뿐이므로 실제로 이 도구를 이 저장소에서 재구현하지 않는다(별도 저장소에서 이미 개발됨) |

### 데이터 파일 스키마 (PoC 호환 — 중요)

`DataPersistence`/`DataMonitor`/`DummyDataGenerator` 3개 PoC가 이미 서로 호환되는 JSON 스키마를 사용하고
있으므로, SampleOrderSystem도 **동일한 필드명·파일명**을 채택한다. 이렇게 하면 옆 저장소의
`DummyDataGenerator.exe`로 이 프로젝트의 `data/` 폴더에 더미 데이터를 생성하거나, `DataMonitor.exe`로
같은 폴더를 실시간 모니터링하는 등 PoC 실행파일을 그대로 붙여 쓸 수 있다.

- `data/samples.json`: 배열, 각 원소 필드 `id`, `name`, `avgProductionMinutesPerUnit`, `yield`, `stock`
- `data/orders.json`: 배열, 각 원소 필드 `orderId`, `sampleId`, `customerName`, `quantity`, `status`
- `status` 값은 대문자 영문 문자열(`RESERVED`/`REJECTED`/`PRODUCING`/`CONFIRMED`/`RELEASED`)로 저장한다.
- 위 필드/파일 구성을 임의로 변경하지 않는다. PRD상 추가 필드(생성 일시 등)가 필요하면 기존 필드는 유지한 채
  **추가만** 한다(PoC 호환성 유지).

## 기술 스택

| 항목 | 선택 |
|---|---|
| 언어 | C++20 |
| 빌드 시스템 | MSBuild (Visual Studio, `.vcxproj` / `.slnx`), PlatformToolset v145 |
| 대상 플랫폼 | Win32 / x64 |
| 애플리케이션 형태 | Console Application |
| 데이터 영속성 | JSON 파일 |
| JSON 처리 | 외부 라이브러리 미사용, PoC(DataPersistence/DataMonitor/DummyDataGenerator)와 동일하게 자체 구현한 `Json::Value`/`Json::FileIO` 사용 |
| 단위 테스트 | GoogleTest/GoogleMock, NuGet 패키지(`gmock`, 1.11.0) 참조(`packages.config`)로 관리 |

현재 저장소에는 Visual Studio 프로젝트 스켈레톤(`SampleOrderSystem.slnx`, `SampleOrderSystem.vcxproj`)만 존재하며
소스 코드는 아직 작성되지 않은 상태다. JSON 처리는 외부 의존성 없이 PoC와 동일한 자체 구현(`Json::Value` 파서/직렬화기,
`Json::FileIO` 파일 read/write)을 새로 작성한다. 단위 테스트(GoogleTest/GoogleMock)는 vcpkg가 아니라 클래식
NuGet 패키지(`gmock` 1.11.0, `packages.config`)로 관리하며, 이 저장소는 vcpkg를 사용하지 않는다.

## 빌드 / 테스트 명령

테스트는 별도 프로젝트(`.Tests.vcxproj`)나 커스텀 MSBuild 프로퍼티로 분리하지 않고, `SampleOrderSystem`
단일 프로젝트의 **빌드 구성(Configuration)** 에 따라 진입점을 조건부 분기한다: `src/main.cpp`가
`#ifdef _DEBUG`(Debug 구성에서 MSVC가 자동 정의)이면 GoogleTest 러너를, 그렇지 않으면(Release 구성,
`NDEBUG`) 실제 앱 진입점을 컴파일한다. 즉 **Debug 빌드 = 단위 테스트 실행, Release 빌드 = 실제 앱 실행**이다.

```powershell
# 단위 테스트 빌드 및 실행 (Debug — tests/*.cpp 포함, GoogleTest 러너로 동작)
msbuild SampleOrderSystem.slnx /p:Configuration=Debug /p:Platform=x64
.\x64\Debug\SampleOrderSystem.exe

# 앱 빌드 및 실행 (Release — 실제 앱 진입점으로 동작)
msbuild SampleOrderSystem.slnx /p:Configuration=Release /p:Platform=x64
.\x64\Release\SampleOrderSystem.exe
```

`packages.config`의 NuGet `gmock` 패키지가 아직 복원되지 않았다면, 작업을 시작하기 전에 먼저 복원한다(임의로 스킵하지 말 것).

## 아키텍처 (MVC)

별도 PoC 저장소(`ConsoleMVC-...` 등)에서 검증된 MVC 구조를 참고하여, 이 저장소에서 아래 구조로 구현한다.

```
src/
  Model/        # Sample, Order, ProductionQueueItem 등 순수 도메인 모델(struct/enum, 로직 없음)
  View/         # 콘솔 입출력 전담 (메뉴 렌더링, 입력 파싱은 하지 않고 표시만)
  Controller/   # 메뉴 흐름 제어, Model과 View를 연결, 도메인 규칙 적용
  Persistence/  # I{Entity}Repository 인터페이스 + Json{Entity}Repository 구현
  Json/         # JsonValue.h/.cpp(Json::Value 클래스), JsonIO.h/.cpp(Json::FileIO 클래스) — 자체 구현 JSON 파서/직렬화기/파일 IO
tests/
  ...           # GoogleTest 기반 단위 테스트
docs/
  PRD.md
```

- Model은 View나 콘솔 입출력에 의존하지 않는다.
- View는 도메인 로직(재고 계산, 상태 전이 등)을 갖지 않는다. 표시만 담당한다.
- Controller가 Model의 도메인 규칙 호출과 View의 출력 호출을 조율한다. Controller는 `<iostream>`에 의존하지 않는다.
- Persistence는 `I{Entity}Repository`(순수 가상 인터페이스, Create/GetAll/FindById/Update/Remove) + `Json{Entity}Repository`(구현체)로 분리한다. 구현체는 생성자에서 `Load()`로 파일을 읽고, CUD 시점마다 `Persist()`로 전체를 다시 기록한다(write-through). 파일이 없거나 파싱에 실패하면 예외를 던지지 않고 빈 목록으로 폴백한다.
- `main.cpp`에서 Repository → Controller → View 순으로 수동 생성자 주입(DI)으로 조립한다.
- 신규 기능은 반드시 이 4계층 분리를 유지하며 추가한다.

## 핵심 도메인 규칙 (요약 — 전체는 PRD 참고)

구현/리뷰 시 아래 규칙을 위반하지 않았는지 항상 확인한다.

- **주문 상태**: `RESERVED → REJECTED` 또는 `RESERVED → (CONFIRMED | PRODUCING) → CONFIRMED → RELEASED`. `REJECTED`는 종단 상태이며 모니터링 집계에서 제외한다.
- **주문 승인 시 재고 판단**: 물리적으로 재고를 차감하지 않는다. 재고 부족/충분 여부를 정확히 판단하기 위해 최신 재고 값만 조회한다.
- **재고의 물리적 증감은 두 시점에만 발생**: 생산 완료 시(+실 생산량), 출고 처리 시(−출고 수량).
- **생산 라인은 단일(1개) 라인**이며, 생산 큐는 **FIFO**로 처리한다. 큐에 진입한 생산 작업은 중간에 취소·변경할 수 없다.
- **실 생산량** = `ceil(부족분 / 수율)`, **총 생산 시간** = `평균 생산시간 × 실 생산량`.
- **생산 시간은 실제 경과 시간을 반영**한다. 앱이 종료되었다가 재시작되어도, 재시작 시점의 현재 시각을 기준으로 그 사이 완료됐어야 할 생산 큐 항목을 정산해야 한다(단순 타이머/백그라운드 스레드 의존 금지).
- **부분 출고는 지원하지 않는다.** 출고는 항상 주문 수량 전체를 한 번에 처리한다.
- 시스템에 등록되지 않은 시료는 주문할 수 없다.

## 코딩 컨벤션

- 클래스/타입: `PascalCase`, 함수/메서드: `PascalCase` 동사형(4개 PoC 컨벤션과 동일 — `PlaceOrder`, `ApproveOrder`, `FindById` 등).
- 헤더/소스 분리(`.h` / `.cpp`)를 지키고, 헤더에는 최소한의 선언만 둔다.
- 예외적인 입력(음수 수량, 미등록 시료 ID 등)은 시스템 경계(사용자 입력 처리 계층)에서만 검증한다. Model 내부 로직은 유효한 값이 들어온다고 가정한다.
- 불필요한 주석을 달지 않는다. 이유(WHY)가 코드만으로 드러나지 않을 때만 짧게 남긴다.
- 단위 테스트 파일명은 테스트 대상 소스 파일명 앞에 `test_`를 붙인다(예: `src/Model/Sample.cpp`를
  테스트하는 파일은 `tests/test_Sample.cpp`).

### Clean Code 원칙

- **함수화(단일 책임)**: 함수/메서드는 하나의 책임만 수행하도록 작게 나눈다. 긴 함수나 중복되는 로직은
  즉시 별도 함수로 추출한다(예: 재고 판정, 실 생산량 계산, 상태 전이 검증은 각각 독립된 함수/메서드로 분리).
- **의미 있는 이름**: 변수/함수명은 축약하지 않고 의도가 그대로 드러나는 이름을 쓴다
  (예: `qty` 대신 `quantity`, `calc` 대신 `calculateActualProductionQuantity`). 이 저장소가 참고하는
  4개 PoC의 네이밍 컨벤션(`CLAUDE.md` "PoC별 참고 포인트" 참고: `PascalCase` 클래스, `I` 접두사 인터페이스,
  `camelCase_` 트레일링 언더스코어 멤버)을 그대로 따른다.
- **매직 넘버/문자열 금지**: 상태값, 임계치 등은 리터럴로 흩뿌리지 않고 `enum class`나 명명된 상수로 정의한다.
- **GoF 디자인 패턴의 목적 있는 적용**: 아래처럼 실제로 복잡성을 줄이는 지점에서 능동적으로 적용을 검토한다.
  - 주문 상태 전이(`RESERVED`/`REJECTED`/`PRODUCING`/`CONFIRMED`/`RELEASED`): State 패턴, 또는 최소한
    전이 테이블 + 검증 함수로 잘못된 전이를 명시적으로 차단.
  - 영속성 계층: Repository 패턴(`I{Entity}Repository` 인터페이스 + `Json{Entity}Repository` 구현, 이미
    4개 PoC와 CLAUDE.md 아키텍처 절에 반영됨).
  - 재고 상태(여유/부족/고갈) 판정처럼 조건별 분기가 늘어나는 로직: Strategy 패턴 고려.
  - 생성 규칙이 복잡한 객체(예: 승인 시점에 파생되는 `ProductionQueueItem` 생성): Factory Method 패턴 고려.
  - **패턴은 목적 없이 남용하지 않는다.** 3줄짜리 로직에 인터페이스·팩토리를 억지로 씌우는 과설계는 금지한다
    — 패턴 적용은 실제로 가독성/확장성을 높이는 경우로 한정한다.

## Git / 커밋 규칙

- 이 저장소에서 커밋을 생성할 때는 항상 **[docs/git_rules/COMMIT_PREVENTION.md](docs/git_rules/COMMIT_PREVENTION.md)** 의 규칙을 따른다.
- 핵심 요약: 의미 단위(Atomic)로 잘게 나누어 커밋, 커밋 전 테스트/빌드 확인, 정해진 커밋 메시지 형식(`type: 요약`) 사용, 커밋 실행 전 항상 변경 내역과 메시지를 사용자에게 먼저 보여주고 확인받는다.
- 규칙과 실제 커밋 방식이 충돌하면 `docs/git_rules/COMMIT_PREVENTION.md`를 따른다.

## 에이전트 기반 개발 워크플로우

이 프로젝트의 실제 구현(Phase 진행)은 `.claude/agents/`에 정의된 서브에이전트 중 아래 3개가 분업한다.
전체 흐름과 각 에이전트의 역할/입출력/피드백 루프는 **[docs/AGENTS.md](docs/AGENTS.md)** 에 정의되어 있다.

| 순서 | 에이전트 | 역할 |
|---|---|---|
| 1 | `phase-developer` | `docs/All_phase_goals.md` 기반으로 `docs_temp/phase_{n}.md` 작성 후 구현, 실패 피드백 시 재작업 |
| 2 | `unit-tester` | 정상/경계·특이 케이스 단위 테스트 작성·실행, 실패 시 phase-developer에게 피드백 |
| 3 | `refactoring-agent` | 3개 Phase가 끝날 때마다 1회, `docs/refactoring_list.md`에 리팩터링 후보 기록 후 수행(동작 변경 없이) |

> `doc-verifier`, `spec-verifier`는 더 이상 워크플로우에서 사용하지 않는다(사용자 결정, 2026-07-15).
> 정의 파일은 `.claude/agents/`에 남아 있지만 호출하지 않는다.

- `phase-developer` ↔ `unit-tester` 루프는 테스트가 모두 통과할 때까지 반복한다.
- `docs_temp/phase_{n}.md`는 Phase별 세부 계획과 재작업 이력을 남기는 작업 문서다(자세한 규칙은 `docs/AGENTS.md`, `docs_temp/README.md` 참고).
- Phase 0/1/2, Phase 3/4/5, ... 처럼 3개 Phase가 연속으로 `unit-tester`까지 통과할 때마다, 다음 Phase로
  넘어가기 전에 `refactoring-agent`를 1회 호출한다. 결과물은 `docs/refactoring_list.md`에 누적 기록된다.

## 참고 문서

- 기능 명세 / 도메인 규칙: [docs/PRD.md](docs/PRD.md)
- 커밋 규칙: [docs/git_rules/COMMIT_PREVENTION.md](docs/git_rules/COMMIT_PREVENTION.md)
- 에이전트 워크플로우: [docs/AGENTS.md](docs/AGENTS.md)
- 리팩터링 목록: [docs/refactoring_list.md](docs/refactoring_list.md)
