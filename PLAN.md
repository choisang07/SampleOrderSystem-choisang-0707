# 반도체 시료 생산주문관리 시스템 - 진행 계획 (PLAN)

관련 문서: [docs/requirement.md](docs/requirement.md), [docs/role.md](docs/role.md)

## 전체 흐름

```
1. PoC 개발 (미션1, 4가지 사례)
        │
        ▼
2. 요구사항 기반 시스템 아키텍처 구성 (PoC 산출물 참고)
        │
        ▼
3. Phase 분리 + Phase별 TDD 루프 (Test → Develope → Review → Test)
        │  (미션2 개발 시 MVC 스켈레톤/영속성 구조는 PoC 산출물 기반으로 작성)
        ▼
   완료
```

---

## 1. PoC 개발

- 대상: requirement.md 미션1의 4가지 항목
- 진행 방식/저장 위치/순서: `poc` Subagent 정의([.claude/agents/poc.md](.claude/agents/poc.md)) 참조 — `Agent(subagent_type: "poc")`로 호출
  1. MVC 스켈레톤 코드 (ConsoleMVC-choisang-0707) — 최우선 진행
  2. 데이터 영속성 처리 / 데이터 모니터링 Tool / Dummy 데이터 생성 Tool — MVC 스켈레톤 확정 후 병렬 진행
- 산출물: 각 PoC 소스 코드(개별 Repository) + 검증 결과 요약(발견한 이슈, 채택한 설계 결정)
- 원격 저장소 (모두 Public):
  | PoC 항목 | 원격 저장소 링크 |
  |---|---|
  | MVC 스켈레톤 코드 | [ConsoleMVC-choisang-0707](https://github.com/choisang07/ConsoleMVC-choisang-0707) |
  | 데이터 영속성 처리 | [DataPersistence-choisang-0707](https://github.com/choisang07/DataPersistence-choisang-0707) |
  | 데이터 모니터링 Tool | [DataMonitor-choisang-0707](https://github.com/choisang07/DataMonitor-choisang-0707) |
  | Dummy 데이터 생성 Tool | [DummyDataGenerator-choisang-0707](https://github.com/choisang07/DummyDataGenerator-choisang-0707) |

## 2. 시스템 아키텍처 구성

- PoC 검증 결과(구조, 영속성 방식, 제약사항)를 반영하여 본 프로젝트(SampleOrderSystem)의 아키텍처를 확정한다.
- 확정 항목:
  - MVC 패키지 구조 (PoC의 ConsoleMVC 구조를 기반으로 확장)
  - 데이터 영속성 방식 (PoC의 DataPersistence 구조를 기반으로 확장)
  - 도메인 모델: Sample(시료 ID, 이름, 평균생산시간, 수율, 재고), Order(주문번호, 시료ID, 고객명, 수량, 상태), 생산 큐(FIFO)
  - 재고/생산 처리 규칙: requirement.md 5.6절의 사례1/사례2 규칙을 아키텍처 레벨에서 반영 (승인 시점 즉시 차감, 생산 완료 시점 일괄 반영, 생산 큐 미고려)
- 산출물: PRD.md (아키텍처/도메인 모델 확정 문서), Harness 셋업(테스트 실행 환경)

## 3. Phase 분리 및 개발

각 Phase는 requirement.md의 기능 명세와 주문 상태 흐름(RESERVED → CONFIRMED/PRODUCING → RELEASE)을 기준으로, 뒤 Phase가 앞 Phase에 의존하는 순서로 구성한다.

| Phase | 범위 | 의존 관계 |
|---|---|---|
| Phase 0 | 준비: PoC 산출물 이식(MVC 스켈레톤/영속성 구조), PRD.md 확정, Harness 셋업 | - |
| Phase 1 | 시료 관리 (등록/조회/검색) | Phase 0 |
| Phase 2 | 주문 접수 (시료 예약 → RESERVED) | Phase 1 |
| Phase 3 | 주문 승인/거절 + 생산 라인 (재고확인, CONFIRMED/PRODUCING/REJECTED 분기, 생산 큐 FIFO, 실생산량 계산, 생산완료 처리) | Phase 2 |
| Phase 4 | 출고 처리 (CONFIRMED → RELEASE) | Phase 3 |
| Phase 5 | 모니터링 (상태별 주문 현황, 재고 현황) | Phase 1~4 전체 |

- Phase 3(주문 승인/거절 + 생산 라인)은 복잡도가 높아 "승인/거절 로직"과 "생산 라인 스케줄러"로 세부 작업을 나눌 수 있다.
- Phase 3의 테스트케이스는 requirement.md 사례1/사례2를 반드시 포함한다.

### Phase별 진행 루프 (TDD)

```
Agent(test)      : Phase 범위의 TestCase 작성 (Edit/Bash 없음 — 코드 수정 불가)
   │
   ▼
Agent(develope)  : TestCase 기반 구현 (PoC 산출물을 MVC/영속성 기반으로 참고)
   │
   ▼
Agent(review)    : 코드 리뷰, ReportFindings로 이상점 보고 (Write/Edit/Bash 없음 — 수정 불가)
   │
   ▼
Agent(develope)  : 이상점 반영
   │
   ▼
Agent(test)      : 회귀 테스트 (해당 Phase + 이전 Phase 전체 재검증)
   │
   ▼
다음 Phase로 이동
```

각 화살표는 서로 다른 Subagent를 `Agent` 도구로 순차 호출하는 것을 의미한다 (이전 단계 산출물을 다음 프롬프트에 명시). 이 루프를 자동 오케스트레이션하려면 `/tdd-role-workflow` 스킬을 사용한다.

- 각 역할의 실제 정의(도구 권한 포함): [docs/role.md](docs/role.md), [.claude/agents/](.claude/agents/)
- Phase가 진행될수록 이전 Phase의 테스트를 포함한 회귀 테스트 스위트를 계속 유지·확장한다 (새 Phase가 이전 Phase의 재고/상태 로직을 깨뜨리지 않는지 확인).

## 참고 원칙

- 미션2(본 프로젝트) 개발 시 MVC/영속성 관련 코드는 PoC가 만든 샘플 코드를 기반으로 작성한다.
- 문서(PRD.md, CLAUDE.md) 관리, Harness 도입, Test, CleanCode, Commit 이력은 requirement.md 미션2 주안점을 따른다.
