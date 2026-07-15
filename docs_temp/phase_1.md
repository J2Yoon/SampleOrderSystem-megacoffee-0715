# Phase 1 — 도메인 모델 정의

## 관련 문서
- `docs/All_phase_goals.md` Phase 1 (도메인 모델 정의)
- `docs/PRD.md` 3절(주문 상태 정의), 4.2.1절(시료 속성), 5절(데이터 모델 요약)

## 목표
View/Controller/저장 방식과 무관한 순수 도메인 모델을 확정한다. 콘솔 입출력, 파일 I/O에 대한 컴파일
의존성이 전혀 없어야 한다(표준 라이브러리 `<string>`, `<chrono>`만 사용).

## 산출물 목록

| 파일 | 내용 | 완료 조건 매핑 |
|---|---|---|
| `src/Model/OrderStatus.h` | `enum class OrderStatus`(Reserved/Rejected/Producing/Confirmed/Released) 및 `IsValidOrderStatusTransition(from, to)` 자유 함수 선언 | PRD 3절 상태 전이 규칙을 전이 테이블로 표현, 잘못된 전이 차단 |
| `src/Model/OrderStatus.cpp` | 허용된 전이 쌍 목록(정적 테이블) 기반 `IsValidOrderStatusTransition` 구현 | RESERVED→REJECTED / RESERVED→CONFIRMED / RESERVED→PRODUCING / PRODUCING→CONFIRMED / CONFIRMED→RELEASED 만 허용, 나머지(REJECTED/RELEASED에서의 전이, 자기 자신으로의 전이 등)는 차단 |
| `src/Model/Sample.h/.cpp` | `Sample` 클래스: 시료 ID, 이름, 평균 생산시간(개당, double), 수율(double), 현재 재고(int). 생성자 + getter. 재고만 `SetStock`으로 갱신 가능(다른 필드는 등록 후 불변) | PRD 4.2.1/5.1 |
| `src/Model/Order.h/.cpp` | `Order` 클래스: 주문번호, 시료 ID, 고객명, 주문 수량, 상태(`OrderStatus`), 생성 일시(`std::chrono::system_clock::time_point`). 생성자(신규 주문은 상태 기본값 RESERVED / 영속성 복원용은 상태 명시), getter, `TryTransitionTo(newStatus)`로만 상태 변경(내부에서 `IsValidOrderStatusTransition` 검사) | PRD 3절/5.2, 잘못된 전이 방지 |
| `src/Model/ProductionQueueItem.h/.cpp` | `ProductionQueueItem` 클래스: 연결 주문번호, 부족분(int), 실 생산량(int), 총 생산 시간(분, double), 생산 시작 시각, 예상 완료 시각(생성자에서 시작 시각 + 총 생산 시간으로 계산). 생성 후 불변(setter 없음 — PRD의 "큐 진입 후 취소·변경 불가" 규칙을 타입으로 보장) | PRD 5.3 |

## 설계 메모
- Model 계층은 순수 데이터 + 자기 자신의 무결성 검증(상태 전이 검증)만 가지며, 재고 판정/생산량 계산 등
  다른 엔티티와 얽힌 로직은 Phase 5/6의 Controller에 위치시킨다(CLAUDE.md Model 순수성 원칙 준수).
- 상태 전이 검증은 State 패턴 대신 "전이 테이블 + 검증 함수"로 구현(현재 상태 종류가 5개, 전이 규칙이
  선형적이라 클래스 계층을 두는 State 패턴은 과설계로 판단).
- 시간 표현은 `std::chrono::system_clock::time_point`(시각)와 분 단위 실수 `duration`(`std::chrono::duration<double, std::ratio<60>>`)을 사용해, 이후 Phase 6에서 실제 경과 시간 계산 시 그대로 재사용 가능하게 한다.
- `ProductionQueueItem`은 setter를 두지 않아 "큐 진입 후 변경 불가" 규칙을 타입 수준에서 강제한다.
- `Sample`은 `SetStock`만 제공(등록 후 이름/평균생산시간/수율은 불변, 재고만 생산완료/출고 시점에 갱신되므로).

## 빌드 반영
- `SampleOrderSystem.vcxproj` / `.vcxproj.filters`에 신규 소스 파일 8개(`.h` 4 + `.cpp` 4: OrderStatus,
  Sample, Order, ProductionQueueItem) 추가.
- Release 구성으로 빌드하여 컴파일 성공 확인(unit-tester가 Debug 구성에서 gtest로 검증 예정이므로 이 단계는
  구현 대상 코드가 실제로 컴파일 가능함만 확인).

## 재작업 이력

### 1차 재작업: vcxproj에 `/utf-8` 컴파일 옵션 추가
- **문제**: Release|x64 최초 빌드가 `C2065`/`C2838`/`C2131` 등으로 실패.
- **원인**: `src/main.cpp`는 Phase 0에서 UTF-8 BOM으로 저장되어 MSVC가 올바르게 해석했지만, 이번에 추가된
  `src/Model/OrderStatus.h`/`.cpp` 등은 BOM 없이 저장되었고 `SampleOrderSystem.vcxproj`에는 애초부터
  `/utf-8` 컴파일러 옵션이 없었다(Phase 0 때부터 파일별 BOM에만 암묵적으로 의존하던 상태). 그 결과 MSVC가
  이 파일들을 시스템 코드페이지(CP949)로 잘못 해석해, 한글 주석 안의 멀티바이트 문자가 주석 종료처럼
  깨지면서 `Confirmed`/`Released` 등 enum 토큰까지 잘못 파싱되어 연쇄 컴파일 에러가 발생함. 도메인 로직
  자체의 버그는 아니었음.
- **조치**: `SampleOrderSystem.vcxproj`의 4개 `ItemDefinitionGroup`(Debug/Release × Win32/x64) 각각의
  `<ClCompile>`에 `<AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>`를 추가해, 파일별
  BOM 유무와 무관하게 항상 UTF-8로 해석되도록 근본적으로 고침(BOM 의존 방식 폐기).
- **재검증 결과**: `Configuration=Release,Platform=x64`와 `Configuration=Debug,Platform=x64` 모두 빌드
  성공(exit code 0). Debug 구성 실행 시 GoogleTest 러너가 `[PASSED] 0 tests`로 정상 종료함을 재확인.
