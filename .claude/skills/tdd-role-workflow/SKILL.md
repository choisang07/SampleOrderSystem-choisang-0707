---
name: TDD Role Workflow
description: Use this skill when developing a Phase of the SampleOrderSystem project, or when the user says things like "Phase N 진행해줘", "이 기능 TDD로 개발해줘", "Test/Develope/Review 루프 돌려줘". Orchestrates the test/develope/review subagents (.claude/agents/) in the loop defined in docs/PLAN.md and docs/role.md.
version: 0.2.0
---

# TDD Role Workflow

## 개요

이 프로젝트(SampleOrderSystem)의 Test / PoC / Develope / Review 역할은 문서가 아니라 실제 **Subagent**(`.claude/agents/TestCodeDeveloper.md`, `poc.md`, `develope.md`, `review.md`)로 정의되어 있다. (Test 역할의 Subagent 이름은 `"test"`가 시스템 예약어와 충돌해 `TestCodeDeveloper`를 사용한다.) ([docs/role.md](../../../docs/role.md)). 이 스킬은 그 중 **test → develope → review → develope → test** 루프를 하나의 Phase 단위로, `Agent` 도구를 이용해 실제로 서로 다른 Subagent를 순차 호출하며 실행하기 위한 절차다. (poc Subagent는 이 루프와 별개로 호출한다.)

시작 전 항상 다음을 먼저 읽는다:
- [docs/PLAN.md](../../../docs/PLAN.md) — Phase 0~5 정의와 전체 흐름
- [docs/PRD.md](../../../docs/PRD.md) — 도메인 모델, 핵심 비즈니스 규칙
- [docs/requirement.md](../../../docs/requirement.md) — 상세 기능 명세, 사례1/사례2
- [docs/role.md](../../../docs/role.md) — 역할별 Agent 정의와 도구 권한

대상 Phase가 명시되지 않았다면, docs/PLAN.md의 Phase 표에서 아직 완료되지 않은 가장 앞선 Phase를 대상으로 삼는다 (사용자에게 확인 후 진행).

## 절차

아래 단계는 **서로 의존적이므로 순차적으로** 실행한다 (앞 단계의 결과를 다음 단계 프롬프트에 넣어야 하므로 병렬 호출 불가). 각 `Agent` 호출은 별도 Subagent이며 이전 대화 맥락을 모르므로, 프롬프트에 대상 Phase, 참고 문서 경로, 앞 단계 산출물(파일 경로/요약)을 반드시 포함한 **self-contained 프롬프트**로 호출한다.

### 0단계 — Phase 브랜치 준비 (`Agent(subagent_type: "develope")`)
- docs/PLAN.md "Phase별 브랜치 전략"에 따라, 대상 Phase의 `phase/{N}-{slug}` 브랜치를 `master`에서 생성/체크아웃하도록 develope 에이전트에게 요청한다(이미 존재하면 체크아웃만). 이후 1~5단계의 모든 커밋은 이 브랜치에서 이뤄진다.

### 1단계 — `Agent(subagent_type: "TestCodeDeveloper")`
- 프롬프트에 포함할 것: 대상 Phase 범위, docs/requirement.md/docs/PRD.md의 관련 섹션, (Phase 3이면) 사례1/사례2 케이스 포함 지시.
- 결과: 작성된 테스트케이스/테스트 파일 경로 목록.

### 2단계 — `Agent(subagent_type: "develope")`
- 프롬프트에 포함할 것: 1단계에서 나온 테스트케이스 파일 경로/요약, 대상 Phase 범위, PoC 산출물 참고 지시(예: ConsoleMVC-choisang-0707, DataPersistence-choisang-0707 경로).
- 결과: 구현된 파일 목록, 테스트 통과 여부(직접 실행한 결과).

### 3단계 — `Agent(subagent_type: "review")`
- 프롬프트에 포함할 것: 2단계에서 구현/수정된 파일 목록, 관련 테스트케이스 경로.
- 결과: `ReportFindings` 형식의 이상점 목록 (없으면 빈 목록).

### 4단계 — `Agent(subagent_type: "develope")` (수정 반영)
- 3단계에서 이상점이 있으면, 그 목록을 프롬프트에 그대로 넣어 반영을 요청한다. 이상점이 없으면 이 단계는 건너뛴다.

### 5단계 — `Agent(subagent_type: "TestCodeDeveloper")` (회귀 검증)
- 프롬프트에 포함할 것: 해당 Phase 테스트 + **이전에 완료된 모든 Phase**의 테스트를 함께 재확인해달라는 요청, 4단계에서 변경된 파일 목록.
- 회귀 결과에 실패가 있으면 3~5단계를 반복한다.

## 완료 후

- 해당 Phase가 모두 통과하면, develope 에이전트에게 `phase/{N}-{slug}` → `master` PR 생성을 요청한다. 머지는 사용자 확인 후 진행한다.
- docs/PLAN.md에 Phase 진행 상태를 표시하고, 다음 Phase로 넘어갈지 사용자에게 확인한다.
- 이 스킬은 한 번에 하나의 Phase만 처리한다. 여러 Phase를 연달아 진행하려면 스킬을 다시 호출한다.
- PoC 작업(`Agent(subagent_type: "poc")`)이 필요할 때는 이 루프와 무관하게 별도로 호출한다.
