# 역할별 작업 규칙 (Role)

이 프로젝트는 아래 4가지 역할로 작업을 분담한다. 각 역할은 문서가 아니라 **실제 Claude Code Subagent**(`.claude/agents/*.md`)로 정의되어 있으며, `Agent` 도구에서 `subagent_type`으로 직접 호출한다.

| 역할 | 목적 | Agent 정의 | 도구 권한 |
|---|---|---|---|
| Test | 테스트케이스 설계 및 작성 | [.claude/agents/test.md](../.claude/agents/test.md) | Read, Grep, Glob, Write, TaskCreate, TaskUpdate (Edit/Bash 없음 — 코드 실행/수정 불가) |
| PoC | 미션1(PoC) 4가지 사례 코드 검증 | [.claude/agents/poc.md](../.claude/agents/poc.md) | Read, Grep, Glob, Bash, Write, Edit, TaskCreate, TaskUpdate (별도 저장소에서만 작업) |
| Develope | Test의 TestCase 기반 실제 기능 개발 | [.claude/agents/develope.md](../.claude/agents/develope.md) | Read, Grep, Glob, Bash, Write, Edit, TaskCreate, TaskUpdate |
| Review | Develope 산출물 리뷰 및 이상점 전달 | [.claude/agents/review.md](../.claude/agents/review.md) | Read, Grep, Glob, ReportFindings (Write/Edit/Bash 없음 — 수정 불가) |

Test와 Review는 **도구 자체가 없어서** 코드나 PRD를 수정할 수 없다 (문서상의 약속이 아니라 시스템적으로 강제되는 제약이다). 발견한 사항은 반드시 결과 보고를 통해 Develope에게 전달한다.

## 역할별 추가 책임 (최근 갱신)

- **Test**: Phase별 테스트케이스를 작성할 때마다 `docs_temp/Phase{N}.md`에 각 테스트케이스 설명을 정리해 추가한다. 콘솔로 재현 가능한 시나리오는 `/system-test` 스킬([.claude/skills/system-test/SKILL.md](../.claude/skills/system-test/SKILL.md))의 케이스 목록에도 추가해, Phase가 진행돼도 이전 Phase의 콘솔 입출력 회귀를 실제로 실행 가능한 상태로 유지한다.
- **Develope**: Test가 작성한 GoogleTest/GoogleMock 유닛 테스트(`Tests/`)를 Debug 구성으로 빌드·실행해 통과 여부를 확인하고(design.md §11.1), `[feat]`/`[refactoring]` 커밋 전에는 반드시 `/system-test` 스킬로 Release 빌드(및 등록된 회귀 케이스가 있다면 통과 여부)도 확인한다(`[test]` 커밋은 제외). 커밋 메시지 규칙은 [CLAUDE.md](../CLAUDE.md) 5절 참고. Phase별 git 브랜치(`phase/{N}-{slug}`) 생성/커밋/PR도 Develope가 전담한다(PLAN.md "Phase별 브랜치 전략").
- **Review**: 요구사항/TestCase 충족 여부와 별개로, [design.md](design.md)에 정의된 디자인 패턴(State/Repository+DIP/Factory) 구조와 규칙이 실제 코드에 지켜지고 있는지도 확인한다.

## 호출 방법

각 역할은 `Agent` 도구로 `subagent_type`을 지정해 호출한다. 예:
- `Agent(subagent_type: "test", prompt: "Phase 1 시료 관리 기능 테스트케이스 작성해줘")`
- `Agent(subagent_type: "develope", prompt: "Phase 1 테스트케이스를 기반으로 시료 관리 기능 구현해줘")`
- `Agent(subagent_type: "review", prompt: "Phase 1 구현 코드 리뷰해줘")`
- `Agent(subagent_type: "poc", prompt: "MVC 스켈레톤 코드 만들어줘")`

이 호출 순서를 자동으로 오케스트레이션하려면 `/tdd-role-workflow` 스킬을 사용한다.

## 역할 간 흐름

```
Test (TestCase 작성)
   │
   ▼
Develope (TestCase 기반 구현)
   │
   ▼
Review (코드 리뷰 → 이상점 보고)
   │
   └──▶ Develope (수정 반영)
             │
             ▼
        Test (회귀 검증)
```

- PoC는 위 흐름과 별도로, [requirement.md](requirement.md)의 미션1에 정의된 4가지 PoC 항목을 독립적으로 검증하는 역할이다.

## 공통 원칙

- 각 Agent는 자신에게 부여된 도구 권한 범위를 벗어난 산출물(코드, 문서)을 만들 수 없다 (권한이 아예 없는 도구는 호출 자체가 불가능하다).
- Test/Review처럼 수정 권한이 없는 Agent는 발견한 이슈를 반드시 결과 보고(또는 Review의 경우 `ReportFindings`)로 전달하고, 실제 반영은 Develope Agent가 수행한다.
- 각 Agent 호출 시 self-contained한 프롬프트를 준다 — Agent는 이전 대화 맥락을 모르므로, 대상 Phase/파일/이전 단계 산출물 등 필요한 정보를 프롬프트에 명시한다.
