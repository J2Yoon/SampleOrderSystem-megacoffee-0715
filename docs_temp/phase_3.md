# Phase 3 — 시료 관리 기능

## 관련 문서
- `docs/All_phase_goals.md` Phase 3 (시료 관리 기능)
- `docs/PRD.md` 4.2(시료 관리 — 4.2.1 속성, 4.2.2 하위 기능)
- `CLAUDE.md` 아키텍처 절(Controller/View 계층 원칙), "PoC별 참고 포인트"(ConsoleMVC 행)

## 목표
Phase 1(`Model::Sample`)과 Phase 2(`Persistence::ISampleRepository`/`JsonSampleRepository`)를 연결하는
첫 MVC 기능(Controller+View)을 구현한다. 시료 등록/조회(재고 포함)/검색(이름 기준) 메뉴를 제공하고,
이후 Phase(주문 접수)에서 "등록되지 않은 시료는 주문 불가"를 검증할 수 있는 기반(`IsSampleRegistered`)을
마련한다.

## 산출물 목록

| 파일 | 내용 | 완료 조건 매핑 |
|---|---|---|
| `src/Controller/SampleController.h/.cpp` | `Controller::SampleRegistrationResult` enum(Success/InvalidInput/DuplicateSampleId), `SampleController` 클래스: `RegisterSample`, `GetAllSamples`, `SearchByName`(이름 부분일치, 대소문자 무시), `IsSampleRegistered`(주문 단계에서 재사용할 미등록 시료 검증 기반) | PRD 4.2.2 등록/조회/검색, DoD의 "미등록 시료 참조 불가 검증 로직" |
| `src/View/ConsoleView.h/.cpp` | 공용 콘솔 I/O 헬퍼(`PrintTitle/PrintLine/PrintDivider/PrintError/ReadLine/ReadInt/ReadDouble`). 이후 Phase(주문/승인/생산/출고/모니터링)에서도 재사용되는 공용 인프라 | CLAUDE.md "콘솔 I/O는 View에만 존재" 원칙 |
| `src/View/SampleView.h/.cpp` | 시료 관리 서브메뉴(등록/목록/검색/뒤로) 표시 및 입력 위임. 도메인 로직 없이 `SampleController` 호출 결과만 표시 | PRD 4.2.2, Controller는 `<iostream>` 미의존, View는 도메인 로직 미보유 |
| `src/main.cpp`(Release 분기) | `JsonSampleRepository` → `SampleController` → `SampleView` 순으로 수동 DI 조립, 시료 관리 서브메뉴 루프 실행(임시. Phase 9에서 `MainMenuController`로 대체 예정) | 수동 실행으로 등록/조회/검색 동작 확인 가능 |

## 설계 결정

### 1. 등록 검증 책임 소재: Controller가 담당
- CLAUDE.md 코딩 컨벤션: "예외적인 입력(음수 수량, 미등록 시료 ID 등)은 시스템 경계(사용자 입력 처리
  계층)에서만 검증한다. Model 내부 로직은 유효한 값이 들어온다고 가정한다."
- `View`는 도메인 지식(수율의 유효 범위 0 초과 1 이하, 평균 생산시간은 양수여야 함 등)을 가지면 안 되므로
  (CLAUDE.md: "View는 도메인 로직을 갖지 않는다"), 콘솔 입력 파싱(문자열→숫자 변환)만 `ConsoleView`가
  담당하고, 값의 유효 범위 검증은 `SampleController::IsValidRegistrationInput`(private static)이
  Model 호출 이전에 수행한다. 즉 "시스템 경계"는 View가 아니라 Controller의 입구로 해석했다 — Controller가
  Model과 Persistence 사이의 유일한 게이트이기 때문이다.
- 중복 ID 판정은 기존 `ISampleRepository::Create`의 반환값(false=중복)을 그대로 재사용해 로직 중복을
  피했다(Phase 2 산출물 재사용).
- 실패 원인을 View가 구분해 안내 메시지를 다르게 표시할 수 있도록 `bool` 대신
  `SampleRegistrationResult`(Success/InvalidInput/DuplicateSampleId) enum을 반환한다. 매직 문자열/불리언
  나열 대신 명명된 결과 타입을 쓰는 것으로 Clean Code 원칙(매직 넘버/문자열 금지)을 따른다. 이 이상의
  패턴(Strategy/State 등)은 로직이 3갈래 분기에 불과해 과설계이므로 도입하지 않는다.

### 2. 시료 검색: 이름 부분 포함(대소문자 무시)
- PRD 4.2.2: "이름 등 속성으로 특정 시료를 검색"만 명시하고 대소문자 처리 방식은 규정하지 않는다. 한글
  이름이 대부분일 것으로 예상되지만, 영문 시료명 검색 시 사용성을 높이기 위해 `std::tolower` 기반의
  대소문자 무시 부분 문자열 매칭을 채택한다(과설계 방지를 위해 정규식/유사도 검색 등은 도입하지 않음).

### 3. `IsSampleRegistered` — Phase 4 선행 준비
- Phase 4(주문 접수) DoD: "미등록 시료 ID에 대한 거부 동작 테스트 통과"를 위해, `OrderController`가 재사용할
  수 있는 조회 API를 `SampleController`에 미리 노출한다(`ISampleRepository::FindById`를 그대로 감싸는 얇은
  래퍼). 별도 검증 클래스를 새로 만들지 않고 기존 Repository 조회를 재사용해 중복을 방지했다.

### 4. `main.cpp` 임시 배선(Phase 9 이전 상태)
- `All_phase_goals.md` Phase 9에서 `MainMenuController`가 전체 메뉴를 통합하기로 되어 있으므로, 이번
  Phase에서 `main.cpp`를 최종 형태로 만들지 않는다. 다만 수동 검증(빌드 후 동작 확인)을 위해 Release 진입점에
  `JsonSampleRepository("data/samples.json")` → `SampleController` → `SampleView`를 조립하고, 시료 관리
  서브메뉴만 반복 실행하는 임시 루프를 둔다. `SampleView::ShowMenuAndHandleSelection()`이 "뒤로(0)" 선택 시
  `false`를 반환하도록 하여 `main`이 이를 종료 신호로 사용한다(Phase 9에서 `MainMenuController`가 여러
  하위 View를 오케스트레이션하도록 교체될 예정이며, 이 반환값 계약도 그때 재검토한다).

## 완료 조건(DoD) 매핑
- 등록되지 않은 시료는 이후 단계에서 참조 불가: `SampleController::IsSampleRegistered` 제공, Phase 4에서
  `OrderController`가 재사용.
- 시료 등록·조회·검색 각각에 대한 컨트롤러 단위 테스트 통과: `SampleController`가 `ISampleRepository&`를
  생성자로 주입받는 구조이므로 unit-tester가 인메모리/스텁 Repository로 테스트 가능(Persistence 계층
  의존 없이 인터페이스만으로 대체 가능).

## 빌드 반영
- `SampleOrderSystem.vcxproj`/`.vcxproj.filters`에 신규 소스 6개(`.h` 3 + `.cpp` 3: SampleController,
  ConsoleView, SampleView) 추가.
- `src/main.cpp`의 Release 분기 수정(신규 include 3개 추가).
- Release/Debug(x64) 양쪽 빌드 성공 확인, 기존 테스트(Phase 0~2) 회귀 없음 확인.

## 재작업 이력
(현재까지 없음)
