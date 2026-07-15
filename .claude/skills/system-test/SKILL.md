---
name: system-test
description: SampleOrderSystem 콘솔 앱의 Release 빌드를 대상으로, 지금까지 누적된 콘솔 입출력 회귀 케이스(시료 등록/조회, 주문 접수/승인/거절, 생산 라인, 모니터링, 출고 등)를 한 번의 빌드로 일괄 실행하고 기대 출력과 비교한다. 사용자가 "system test 해줘", "system test 돌려줘", "회귀 테스트 돌려줘", "빌드 확인해줘" 등으로 요청할 때, 또는 [feat]/[refactoring] 커밋 전 빌드 확인이 필요할 때 사용한다.
---

# SampleOrderSystem System Test

CLAUDE.md 5절(커밋 메시지 규칙)과 PLAN.md의 "Phase가 진행될수록 이전 Phase의 테스트를 포함한
회귀 테스트 스위트를 계속 유지·확장한다"는 원칙을 실제로 실행 가능하게 만드는 스킬이다.
test 에이전트가 Phase별로 추가한 콘솔 입출력 케이스를
`.claude/skills/system-test/run.ps1`의 `$cases` 배열에 누적해두고, 이 스킬은 그 케이스들을
**Release 빌드 1회 + 실행 N회**로 재현한다(케이스마다 다시 빌드하지 않는다).

## 역할 분담 (중요)

- **test 에이전트**: Phase별 TestCase를 작성할 때, requirement.md 기반 시나리오를 콘솔
  입력/기대 출력 형태로도 함께 정리해 이 스킬의 `run.ps1` 안 `$cases` 배열에 `Write`로
  추가한다. test 에이전트는 Bash가 없어 이 스크립트를 직접 실행할 수 없다 — 케이스 작성까지만
  담당한다.
- **develope 에이전트 / 메인 에이전트**: 실제 빌드·실행은 이 스킬로 수행한다. 특히
  `[feat]`/`[refactoring]` 커밋 전에는 반드시 이 스킬(최소한 빌드 성공 여부)을 실행해 확인한다
  (`[test]` 커밋은 테스트 코드/케이스만 추가하는 것이라 제외).

## 실행 방법

PowerShell 도구로 아래 스크립트를 실행한다:

```
powershell -ExecutionPolicy Bypass -File .claude/skills/system-test/run.ps1
```

스크립트가 하는 일:
1. MSBuild로 `Project17.slnx`를 `Release|x64` 구성으로 1회 빌드한다. 빌드가 실패하면 그 자리에서
   중단하고 케이스를 실행하지 않는다(exit code 0이 아니면 실패로 간주).
2. 스크립트 안의 `$cases` 배열에 정의된 각 케이스를 이미 빌드된 `x64/Release/Project17.exe`에
   콘솔 입력(줄 단위)으로 그대로 넣어 실행하고 출력을 캡처한다. `InputLines`가 여러 줄이면 실제
   콘솔에서 한 줄씩 Enter를 치는 것과 동일하게 순서대로 입력한다.
3. 캡처된 출력에서 빈 줄을 제거한 뒤 `Expect`와 비교해 PASS/FAIL을 표로 출력한다.
4. 실패 개수를 프로세스 종료 코드로 반환한다(0이면 전부 PASS).

`$cases`가 비어 있으면(Phase 0 초기 등) 빌드 성공 여부만 확인하고 종료한다 — 즉 케이스가 아직
없어도 "빌드가 되는지"만 확인하는 용도로 그대로 쓸 수 있다.

이미 최신 Release 빌드가 있고 다시 빌드할 필요가 없다면 `-SkipBuild` 옵션을 붙여서 실행 단계만
반복할 수 있다:

```
powershell -ExecutionPolicy Bypass -File .claude/skills/system-test/run.ps1 -SkipBuild
```

## 결과 보고 방법

- 스크립트가 출력한 표를 그대로 사용자에게 보여준다.
- FAIL이 있으면 해당 케이스의 Category/Input/Expect/Actual을 짚어서 어떤 입력이 왜 다르게
  나왔는지 설명한다.
- 빌드 자체가 실패했다면(케이스 실행 전 중단) 빌드 에러 메시지를 그대로 보여준다.
- 전부 PASS면 몇 개 케이스가 통과했는지만 간단히 요약한다.

## 새 케이스 추가하기 (test 에이전트 담당)

Phase별 TestCase를 requirement.md 기준으로 도출한 뒤, 콘솔로 재현 가능한 시나리오는
`run.ps1`의 `$cases` 배열 끝에 같은 해시테이블 형식으로 추가한다:

```powershell
@{ Category = "<분류명>"; InputLines = @('<한 줄씩 입력할 값>', ...); Expect = "<기대 출력(빈 줄 제외, 여러 줄이면 개행으로 구분)>" }
```

- 한 줄짜리 입력이면 `InputLines`에 원소 하나만 넣는다.
- 시료 등록 → 주문 접수 → 승인처럼 여러 메뉴를 거치는 시나리오는 실제 사용자가 콘솔에 입력하는
  순서 그대로 배열 원소를 나열한다.
- `Expect`는 실제 출력되는 값만 적는다(입력 프롬프트 등 매 실행마다 달라지지 않는 고정 출력만
  비교 대상으로 삼는다).

정확한 메시지가 아니라 **"뭐든 에러만 나면 통과"**하는 케이스(예: 존재하지 않는 시료 ID로 주문,
잘못된 메뉴 번호 입력)는 `Expect` 대신 `ExpectError = $true`를 쓴다:

```powershell
@{ Category = "<분류명>"; InputLines = @('<에러가 나야 하는 입력>', ...); ExpectError = $true }
```

이 타입은 stderr(에러 메시지)가 비어있지 않은지만 확인하고, 어떤 문구인지는 검사하지 않는다.

requirement.md 5.6절 **사례1/사례2**(승인 즉시 재고 차감, 생산 큐 미고려로 인한 중복 생산)처럼
도메인 핵심 규칙을 검증하는 케이스는 반드시 이 회귀 세트에 포함한다.

케이스를 추가/수정한 뒤에는 커밋할지 사용자에게 확인한다 — `run.ps1`과 케이스 목록은 프로젝트에
커밋되어 이후 모든 Phase가 동일한 회귀 세트를 쓰게 되므로, 회귀 케이스 추가 자체도 Review
대상으로 취급한다.
