# CLAUDE.md

이 파일은 Claude Code가 이 저장소에서 작업할 때 참고하는 프로젝트 가이드입니다.
기능 요구사항과 도메인 규칙의 원본은 항상 **[docs/PRD.md](docs/PRD.md)** 이며, 이 문서와 PRD가 상충하면 PRD를 따른다.

## 프로젝트 개요

"반도체 시료 생산주문관리 시스템" — 반도체 시료(Sample)의 등록, 주문 접수/승인/거절, 생산, 출고 전 과정을
콘솔에서 관리하는 애플리케이션. 상세 기능 명세, 상태 전이, 계산식은 `docs/PRD.md` 참고.

## 기술 스택

| 항목 | 선택 |
|---|---|
| 언어 | C++20 |
| 빌드 시스템 | MSBuild (Visual Studio, `.vcxproj` / `.slnx`), PlatformToolset v145 |
| 대상 플랫폼 | Win32 / x64 |
| 애플리케이션 형태 | Console Application |
| 데이터 영속성 | JSON 파일 |
| 단위 테스트 | GoogleTest (gtest) |

현재 저장소에는 Visual Studio 프로젝트 스켈레톤(`SampleOrderSystem.slnx`, `SampleOrderSystem.vcxproj`)만 존재하며
소스 코드는 아직 작성되지 않은 상태다. 의존성(JSON 라이브러리, gtest)은 vcpkg로 관리하는 것을 기본으로 한다.

## 빌드 / 테스트 명령

```powershell
# 빌드 (x64 Debug)
msbuild SampleOrderSystem.slnx /p:Configuration=Debug /p:Platform=x64

# 실행
.\x64\Debug\SampleOrderSystem.exe

# 테스트 프로젝트가 구성된 이후 (예: SampleOrderSystem.Tests)
vstest.console.exe .\x64\Debug\SampleOrderSystem.Tests.dll
```

테스트 프로젝트나 vcpkg 매니페스트가 아직 없다면, 작업을 시작하기 전에 먼저 구성한다(임의로 스킵하지 말 것).

## 아키텍처 (MVC)

PoC 단계에서 검증한 MVC 스켈레톤 구조를 그대로 따른다.

```
src/
  Model/        # Sample, Order, ProductionQueueItem 등 도메인 모델 + 저장소(Repository) 인터페이스
  View/         # 콘솔 입출력 전담 (메뉴 렌더링, 입력 파싱은 하지 않고 표시만)
  Controller/   # 메뉴 흐름 제어, Model과 View를 연결, 도메인 규칙 적용
  Persistence/  # JSON 파일 read/write, 데이터 로드·저장
tests/
  ...           # GoogleTest 기반 단위 테스트
docs/
  PRD.md
```

- Model은 View나 콘솔 입출력에 의존하지 않는다.
- View는 도메인 로직(재고 계산, 상태 전이 등)을 갖지 않는다. 표시만 담당한다.
- Controller가 Model의 도메인 규칙 호출과 View의 출력 호출을 조율한다.
- 신규 기능은 반드시 이 3계층 분리를 유지하며 추가한다.

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

- 클래스/타입: `PascalCase`, 함수/메서드: `camelCase` 또는 프로젝트 내 기존 관례를 따른다(첫 파일 작성 시 일관성 있게 확정).
- 헤더/소스 분리(`.h` / `.cpp`)를 지키고, 헤더에는 최소한의 선언만 둔다.
- 예외적인 입력(음수 수량, 미등록 시료 ID 등)은 시스템 경계(사용자 입력 처리 계층)에서만 검증한다. Model 내부 로직은 유효한 값이 들어온다고 가정한다.
- 불필요한 주석을 달지 않는다. 이유(WHY)가 코드만으로 드러나지 않을 때만 짧게 남긴다.

## Git / 커밋 규칙

- 이 저장소에서 커밋을 생성할 때는 항상 **[docs/git_rules/COMMIT_PREVENTION.md](docs/git_rules/COMMIT_PREVENTION.md)** 의 규칙을 따른다.
- 핵심 요약: 의미 단위(Atomic)로 잘게 나누어 커밋, 커밋 전 테스트/빌드 확인, 정해진 커밋 메시지 형식(`type: 요약`) 사용, 커밋 실행 전 항상 변경 내역과 메시지를 사용자에게 먼저 보여주고 확인받는다.
- 규칙과 실제 커밋 방식이 충돌하면 `docs/git_rules/COMMIT_PREVENTION.md`를 따른다.

## 참고 문서

- 기능 명세 / 도메인 규칙: [docs/PRD.md](docs/PRD.md)
- 커밋 규칙: [docs/git_rules/COMMIT_PREVENTION.md](docs/git_rules/COMMIT_PREVENTION.md)
