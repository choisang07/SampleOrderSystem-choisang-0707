# CLAUDE.md

이 파일은 이 저장소(SampleOrderSystem 본 프로젝트)에서 작업하는 Claude Code에게 제공하는 가이드입니다.

## 프로젝트 개요

반도체 시료 생산주문관리 시스템(SampleOrderSystem)을 콘솔 기반으로 개발하는 개인과제 프로젝트입니다.
- 상세 요구사항: [docs/requirement.md](docs/requirement.md)
- 제품 요구사항 정의: [docs/PRD.md](docs/PRD.md)
- 아키텍처 설계(패키지 구조, 디자인 패턴, JSON 스키마): [docs/design.md](docs/design.md)
- 진행 계획(Phase, 워크플로우): [docs/PLAN.md](docs/PLAN.md)
- 역할 정의: [docs/role.md](docs/role.md) (개요), 실제 Subagent 정의는 [.claude/agents/](.claude/agents/) 하위 (test.md, poc.md, develope.md, review.md)

작업을 시작하기 전 항상 위 5개 문서(특히 docs/PLAN.md, docs/role.md)를 먼저 확인한다.

## 작업 원칙

### 1. PoC와 본 프로젝트는 분리한다
- 미션1(PoC) 4가지 항목(MVC 스켈레톤, 데이터 영속성, 데이터 모니터링 Tool, Dummy 데이터 생성 Tool)은 **이 디렉토리(SampleOrderSystem 본 프로젝트) 안에서 작업하지 않는다.**
- PoC는 이 프로젝트 폴더 기준 상위 디렉토리에 형제 폴더로, 각각 독립된 git 저장소로 만든다.
  - `../ConsoleMVC-choisang-0707`, `../DataPersistence-choisang-0707`, `../DataMonitor-choisang-0707`, `../DummyDataGenerator-choisang-0707`
- 본 프로젝트(SampleOrderSystem) 개발 시 MVC 구조/영속성 처리 코드는 위 PoC 결과물을 기반으로 이식/확장한다.

### 2. 역할 기반 TDD로 진행한다 (Subagent 기반)
- 4가지 역할: **test / poc / develope / review**. 각각 실제 Claude Code Subagent로 정의되어 있다 ([.claude/agents/](.claude/agents/)). `Agent` 도구에서 `subagent_type: "test"`처럼 직접 호출한다.
- test와 review Subagent는 Edit/Write(review는 Write도 없음)와 Bash가 없어 **코드나 PRD를 수정할 수 없다** — 시스템적으로 강제되는 제약이다. 발견한 사항은 결과 보고(review는 `ReportFindings`)로 develope에게 전달한다.
- Phase별 진행 루프: `test(TestCase 작성) → develope(구현) → review(이상점 보고) → develope(반영) → test(회귀)`. 각 Agent 호출은 이전 대화를 모르므로 self-contained 프롬프트로 호출한다.
- Phase 구성은 [docs/PLAN.md](docs/PLAN.md)의 Phase 0~5 표를 따른다 (시료 관리 → 주문 접수 → 승인/거절+생산라인 → 출고 → 모니터링 순).
- 브랜치는 역할별이 아니라 **Phase별**로 나눈다(`phase/{N}-{slug}`, git 작업은 Bash를 가진 develope 에이전트만 수행). 회귀 통과 후 `master`로 PR을 생성/머지한다. 자세한 내용은 [docs/PLAN.md](docs/PLAN.md)의 "Phase별 브랜치 전략" 참고.
- 이 흐름을 자동으로 오케스트레이션하려면 `/tdd-role-workflow` 스킬을 사용한다.

### 3. 핵심 비즈니스 규칙 (재고/생산)
- 승인 시 기존 재고 소진분은 **승인 즉시** 차감, 신규 생산분은 **생산 완료 시점**에 일괄 반영한다.
- 실 생산량 = `ceil(부족분 / 수율)`이며, 생산량에는 수율 손실을 재적용하지 않는다 (전량 정상 생산 처리).
- 재고 확인 시 생산 큐는 절대 고려하지 않는다 (다른 주문이 예정한 미래 생산분은 무시). 자세한 근거는 [docs/requirement.md](docs/requirement.md) 5.6절 사례1/사례2 참고.

### 4. 문서 최신화
- 요구사항/아키텍처/계획에 변경이 생기면 docs/requirement.md, docs/PRD.md, docs/PLAN.md 중 해당 문서를 함께 갱신한다.
- Review 결과는 `docs/reviews/`, Test 케이스는 test Subagent 정의([.claude/agents/test.md](.claude/agents/test.md))의 산출물 규칙을 따라 기록한다.

### 5. 커밋 메시지 규칙
- 커밋 메시지는 `[타입] 내용` 형식으로 작성한다 (예: `[feat] 시료 등록 기능 추가`).
- 타입 태그(`[feat]` 등)를 제외한 나머지 내용(제목, 본문 모두)은 **한글로 작성한다**.
- 타입 목록:

| 타입 | 용도 |
|---|---|
| `[feat]` | 신규 기능 개발 |
| `[refactoring]` | 리팩토링 (동작 변경 없는 구조 개선) |
| `[test]` | TestCase 추가/수정 |
| `[fix]` | 버그 수정 (Review 이상점 반영, 회귀 테스트 실패 수정 등) |
| `[docs]` | CLAUDE.md/docs/PRD.md/docs/PLAN.md/docs/design.md 등 문서 변경 |
| `[chore]` | 빌드 설정, .gitignore, 디렉토리 정리 등 기능과 무관한 변경 |

- 하나의 커밋에 여러 타입이 섞이면 가장 주된 목적 하나로 태깅한다(예: 기능 추가 중 사소한 문서 갱신이 포함되면 `[feat]`).
- `[feat]`/`[refactoring]` 커밋 전에는 `/system-test` 스킬([.claude/skills/system-test/SKILL.md](.claude/skills/system-test/SKILL.md))로 빌드(및 등록된 회귀 케이스가 있다면 그 통과 여부)를 확인한다. `[test]` 커밋은 케이스 추가만 하는 것이므로 제외한다.
