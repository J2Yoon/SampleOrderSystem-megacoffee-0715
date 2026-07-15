# 에이전트 워크플로우 (Agent Workflow)

이 프로젝트의 개발은 3개의 전문화된 서브에이전트가 아래 순서와 피드백 루프로 협업하여 진행한다.
각 에이전트는 `.claude/agents/`에 실제 subagent 정의 파일(`.md`)로 등록되어 있으며, Claude Code의
Agent 툴에서 `subagent_type`으로 이름을 지정해 호출한다. 에이전트 간 호출·루프 반복은 **에이전트 스스로가 아니라
메인 세션(오케스트레이터)이 관리**한다 — 즉 phase-developer와 unit-tester가 서로를 직접 호출하지 않고,
메인 세션이 둘 사이를 오가며 결과를 전달한다.

> `doc-verifier`, `spec-verifier`는 더 이상 워크플로우에서 사용하지 않는다(사용자 결정, 2026-07-15).
> 문서 정합성/최종 스펙 검증은 phase-developer의 자체 점검과 메인 세션의 판단으로 갈음한다.
> `.claude/agents/doc-verifier.md`, `.claude/agents/spec-verifier.md` 정의 파일 자체는 남겨두되 호출하지 않는다.

## 전체 흐름

```
[1] phase-developer ── docs_temp/phase_{n}.md 작성 + 코드 구현
         │
         v
[2] unit-tester ── 정상 케이스 + 특이/경계 케이스 테스트 작성·실행
         │
    ┌────┴────┐
  실패        통과
    │           │
    v           v
 [1]로     다음 Phase로
 피드백
 (원인분석
  후 재수정)
                │
                v
  (3개 Phase마다 1회) [3] refactoring-agent
      ── docs/refactoring_list.md 갱신 + 리팩터링 수행
                │
                v
           그다음 Phase로
```

`refactoring-agent`는 매 Phase마다 호출되는 것이 아니라, **Phase 0/1/2, Phase 3/4/5, Phase 6/7/8, ...
와 같이 3개 Phase가 연속으로 unit-tester까지 통과할 때마다 1회** 메인 세션이 호출한다.

## 1. phase-developer — 개발 에이전트

- **역할**: `docs/All_phase_goals.md`에서 현재 진행할 Phase를 확인하고, 해당 Phase의 세부 실행 계획을
  `docs_temp/phase_{n}.md`로 작성한 뒤, 그 계획에 따라 실제 코드를 구현한다.
- **입력**: `docs/All_phase_goals.md`, `docs/PRD.md`, `CLAUDE.md`, (재작업 시) unit-tester의 실패 리포트
- **출력**: `docs_temp/phase_{n}.md`(세부 계획 + 진행/수정 이력), `src/` 하위 구현 코드
- **재작업 트리거**: unit-tester로부터 실패 피드백을 받으면, 실패 원인을 분석하고 코드를 수정한 뒤 다시
  unit-tester의 검증을 받도록 메인 세션에 결과를 반환한다.
- **특징**: `docs_temp/phase_{n}.md`에 "무엇을 왜 수정했는지"를 재작업할 때마다 갱신하여 이력을 남긴다.

## 2. unit-tester — 유닛 테스트 에이전트

- **역할**: phase-developer가 구현한 코드에 대해 **정상 동작 케이스**뿐 아니라 **경계값/예외/특이 케이스**까지
  포함한 단위 테스트를 작성하고 실행한다.
- **입력**: phase-developer가 만든 `src/` 코드, `docs/PRD.md`(도메인 규칙 검증 기준), `docs_temp/phase_{n}.md`
- **출력**: `tests/` 하위 테스트 코드, 테스트 실행 결과 요약(통과/실패 개수, 실패 시 재현 조건과 원인 추정)
- **판정**:
  - 실패 시 → phase-developer가 원인을 분석할 수 있도록 실패 테스트명, 입력값, 기대값, 실제값을 구체적으로 정리해 반환한다.
  - 통과 시 → 해당 Phase가 완료된 것으로 보고 다음 Phase로 넘어간다.

## 3. refactoring-agent — 주기적 리팩터링 에이전트

- **역할**: 3개 Phase가 끝날 때마다(Phase 0/1/2, Phase 3/4/5, ... 단위) 그동안 구현된 코드에서 Clean Code/
  CLAUDE.md 컨벤션을 벗어난 부분을 찾아 `docs/refactoring_list.md`에 기록하고, 실제로 리팩터링을 수행한다.
  동작을 바꾸지 않는 리팩터링만 수행하며(공개 인터페이스/파일 스키마/도메인 규칙 변경 금지), 수정 전후로
  반드시 빌드·테스트를 재확인한다.
- **입력**: 지금까지 구현된 `src/` 코드, `docs/refactoring_list.md`(기존 미완료 항목), `CLAUDE.md` "Clean Code
  원칙" 절
- **출력**: `docs/refactoring_list.md` 갱신(신규 후보 추가 + 완료 항목 `[x]` 표시), 리팩터링된 `src/` 코드
- **실행 시점**: unit-tester가 3번째 연속 Phase까지 통과시킨 직후, 다음 Phase로 넘어가기 전에
  메인 세션이 1회 호출한다.
- **특징**: 새 기능을 추가하지 않는다. 테스트가 깨지면 해당 항목을 되돌리고 `docs/refactoring_list.md`에
  사유를 남긴다(무리하게 밀어붙이지 않는다).

## 피드백 루프 규칙

- **unit-tester ↔ phase-developer** 루프는 테스트가 모두 통과할 때까지 반복한다.
- 모든 재작업 이력은 `docs_temp/phase_{n}.md`에 반영 사항으로 기록한다.
- 커밋은 `docs/git_rules/COMMIT_PREVENTION.md` 규칙을 따르며, 하나의 Phase가 unit-tester까지 통과한 뒤 의미 단위로 커밋한다.

## docs_temp/ 디렉터리

- Phase별 세부 실행 계획과 재작업 이력(phase-developer가 작성)이 위치하는 작업 문서 디렉터리.
- 파일명 규칙: `docs_temp/phase_{n}.md` (예: `phase_1.md`, `phase_6.md`)
- 완료된 Phase의 계획 문서도 삭제하지 않고 이력으로 남긴다.

## docs/refactoring_list.md

- `refactoring-agent`가 발견한 리팩터링 후보와 처리 이력을 남기는 문서. `[ ]`(미완료)/`[x]`(완료)로
  항목을 표시하며, 형식은 `.claude/agents/refactoring-agent.md`의 "문서 형식" 절을 따른다.
- 완료되지 않은 항목은 삭제하지 않고 다음 `refactoring-agent` 실행 때 이어서 처리한다.
