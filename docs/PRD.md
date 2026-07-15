# PRD - 반도체 시료 생산주문관리 시스템 (SampleOrderSystem)

관련 문서: [requirement.md](requirement.md) (상세 기능 명세) · [PLAN.md](PLAN.md) (진행 계획) · [role.md](role.md) (역할 규칙)

## 1. 개요

가상의 반도체 회사 "S-Semi"의 시료(Sample) 생산/주문을 콘솔 기반으로 관리하는 시스템. 엑셀/메모장 기반 관리로 인한 주문 누락, 재고-공정 현황 파악 어려움을 해결하는 것이 목표.

- 제출 대상 Repository명: `SampleOrderSystem-choisang-0707`
- 실행 방식: 콘솔(CLI), 담당자가 메뉴 선택 및 명령 입력

## 2. 도메인 모델

| 엔티티 | 속성 |
|---|---|
| Sample (시료) | 시료 ID, 이름, 평균 생산시간, 수율, 현재 재고 수량 |
| Order (주문) | 주문번호, 시료 ID, 고객명, 주문 수량, 상태(RESERVED/REJECTED/PRODUCING/CONFIRMED/RELEASE) |
| ProductionQueue (생산 큐) | FIFO로 대기 중인 PRODUCING 주문 목록 |

## 3. 주문 상태 정의

| 상태 | 의미 |
|---|---|
| RESERVED | 주문 접수 |
| REJECTED | 주문 거절 (모니터링 제외) |
| PRODUCING | 승인 완료, 재고 부족으로 생산 중 |
| CONFIRMED | 승인 완료, 출고 대기 중 |
| RELEASE | 출고 완료 |

## 4. 핵심 비즈니스 규칙 (재고/생산)

requirement.md 5.6절 및 사례1/사례2 기준:

1. 주문 승인 시 **기존 재고로 충당 가능한 만큼은 승인 즉시** 차감한다 (생산 완료를 기다리지 않음).
2. 새로 생산된 수량은 **해당 생산 작업이 완료되는 시점**에 한 번에(주문/생산 단위로) 재고에 반영된다.
3. 실 생산량 = `ceil(부족분 / 수율)`. 생산된 수량은 수율 손실 없이 전량 정상 처리된다 (수율은 생산량 산정에만 사용).
4. 생산 소요 시간은 현실 시간(wall-clock) 기준이며, 프로그램 재시작과 무관하게 진행 상태가 영속된다.
5. 재고 확인 시 생산 큐(다른 주문이 미래에 생산 완료로 채울 예정 수량)는 고려하지 않는다 — 오직 현재 시점의 재고 수치만 사용한다. 이로 인해 등록 시각이 앞선 주문의 생산 완료 시각보다 늦게 도착한 주문은 이미 선점된 재고를 보고 중복 생산이 발생할 수 있으며, 이는 의도된 동작이다.

## 5. 기능 범위 (requirement.md 5장 상세 참조)

- 메인 메뉴 (요약 현황 표시)
- 시료 관리 (등록/조회/검색)
- 시료 주문 (예약)
- 주문 승인/거절
- 모니터링 (주문량/재고량)
- 생산 라인 (생산 현황, 대기 큐 FIFO)
- 출고 처리

## 6. 아키텍처 방향

- PoC(미션1) 4가지 산출물(별도 Repository, `poc` Subagent 정의 [.claude/agents/poc.md](../.claude/agents/poc.md) 참조) 검증 결과를 반영하여 확정한다. 즉 **PoC 선행 → 아키텍처 확정** 순서.
- MVC 구조: PoC의 `ConsoleMVC-choisang-0707` 구조를 기반으로 확장.
- 데이터 영속성: PoC의 `DataPersistence-choisang-0707` 구조를 기반으로 확장. JSON 파일 방식(nlohmann/json 사용)으로 결정.
- 데이터 모니터링/Dummy 데이터 생성 도구는 PoC 산출물을 참고하여 본 프로젝트의 관리자 도구로 통합.
- 확정된 아키텍처 세부 내용은 PoC 완료 후 본 문서 6장을 갱신한다.

## 7. 개발 진행 방식

- Phase 분리 및 Phase별 test → develope → review → develope → test(회귀) 루프: [PLAN.md](PLAN.md) 참조
- 역할별 책임/권한(실제 Subagent 정의): [role.md](role.md), [.claude/agents/](../.claude/agents/)

## 8. 품질 기준 (미션2 주안점)

1. CLAUDE.md, PRD.md 등 문서 최신 상태 유지
2. Harness(테스트 자동 실행 환경) 도입 — 유닛 테스트(GoogleTest/GoogleMock) + 시스템 테스트(`/system-test`) 2계층, 자세한 내용은 [design.md](design.md) §11 참고
3. Test: Phase별 TestCase 기반 TDD
4. CleanCode 준수
5. 의미 단위의 Commit 이력 관리
