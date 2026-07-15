# Phase 4 특이사항 기록 (요구사항/설계 모호성, 구현 전 가정)

Test 에이전트는 Edit/Bash로 프로덕션 코드나 PRD/요구사항 문서를 수정하지 않고 아래 항목을 기록만 한다.
[testcase.md](testcase.md)에서 "가정"으로 표시한 것들의 근거를 여기 모았다. Phase 1의 abnormal.md 형식(확정 결과/영향받는 테스트케이스 명시)을 그대로 따른다.

## 1. `ReleaseService`가 `ISampleRepository`(재고 저장소)를 아예 받지 않아도 되는가 — 설계 근거는 있으나 Develope 구현 전이라 미확정

- `docs/requirement.md` 5.6절 규칙: "재고 반영은... 승인 시점(기존 재고 충당)/생산 완료 시점(신규 생산분)에만 처리된다" — 출고 시점 재고 반영은 명시적으로 없음.
- `docs/requirement.md` 5.7절: "재고가 충분해진 CONFIRMED 주문에 대해 출고 처리 / 출고 실행 → 주문 상태 RELEASE로 전환" — 상태 전이만 언급, 재고 언급 없음.
- `docs/design.md` §4.1(84~90행) `IOrderState` 인터페이스 정의:
  - `approve(Order&, SampleService&, ProductionService&)` — 재고/생산 서비스를 명시적으로 인자로 받음(승인 시 재고 차감/생산 큐 등록이 필요하므로).
  - `release(Order& order)` — **`Order` 하나만 인자로 받음**. `SampleService`/`ISampleRepository` 등 재고 관련 의존성이 인터페이스 시그니처 자체에 없다.
- 이 인터페이스 설계(§4.1)를 그대로 따르면, `ConfirmedState::release`뿐 아니라 이를 감싸는 `ReleaseService`도 애초에 재고 저장소에 접근할 방법이 없는 것이 자연스러운 해석이다.

### 이번 문서에서 채택한 가정 (미확정 — Develope 구현 시 재검토 필요)
- **`ReleaseService(IOrderRepository& orderRepo)`로 재고 의존성 없이 구현하는 것으로 가정**하고 `testcase.md`/`Tests/ReleaseServiceTest.cpp`를 작성했다. 이 경우 "출고 시 재고 미접촉"(TC-P4-002)은 Mock의 `Times(0)` 런타임 검증이 아니라, **애초에 `ISampleRepository` 파라미터 자체가 없다는 컴파일 타임 계약**으로 더 강하게 보장된다.
- **대안(기각하지 않고 병기)**: `MainController`의 서비스 조립(wiring) 편의를 위해 `ReleaseService`도 다른 Service들과 동일하게 `ISampleRepository&`를 생성자에서 받되 실제로는 사용하지 않는 방식으로 구현할 수도 있다. 이 경우 `docs/design.md` §4.1의 `IOrderState::release(Order&)` 시그니처(재고 의존성 없음)와는 다소 어긋나지만, 실용적으로 흔한 패턴이다. 이 대안을 Develope가 채택한다면, `Tests/ReleaseServiceTest.cpp`에 `MockSampleRepository`를 추가하고 `EXPECT_CALL(sampleRepo, save(_)).Times(0)`을 반드시 추가해 회귀 검증해야 한다(원 요청 프롬프트가 제시한 방식).
- **우선순위: Medium.** 요구사항/설계 문서상 재고 미접촉이라는 "결과"에는 이견이 없고(5.6절, 5.7절, §4.1 모두 일관됨), 다만 그 결과를 "타입 시스템으로 원천 차단"할지 "런타임 Mock 검증"으로 확인할지의 **구현 방식** 선택만 남아 있다. 어느 쪽을 택해도 사용자 확인 없이 Develope가 결정 가능하다고 판단해 별도 확인 요청 없이 이 문서에 기록만 하고 진행했다. Develope가 실제 구현 시그니처를 확정하면 이 문서와 `testcase.md`/`Tests/ReleaseServiceTest.cpp`를 회귀 단계에서 그에 맞춰 갱신한다(Phase 1의 ID 포맷 정정 사례와 동일한 패턴).

## 2. 출고 실패 시 예외 타입(`std::invalid_argument` 통일) — Phase 1 관례를 그대로 연장

- `docs/design.md`에는 `ReleaseService`의 예외 타입에 대한 명시가 없다.
- Phase 1의 `SampleFactory`/`SampleService`가 유효성 위반 시 전부 `std::invalid_argument`를 던지는 관례가 이미 확립되어 있다(`docs_temp/Phase1/testcase.md` E절, `Tests/SampleServiceTest.cpp`).

### 확정 결과 (Phase 1 관례 연장, 별도 확인 불요로 판단)
- 상태 가드 위반(비-CONFIRMED 출고 시도)과 존재하지 않는 주문번호 모두 **`std::invalid_argument`**로 통일해 테스트를 작성했다(TC-P4-010~013, TC-P4-020). Develope가 다른 예외 타입(`std::runtime_error`, `std::logic_error` 등)을 쓰기로 결정하면 회귀 단계에서 `Tests/ReleaseServiceTest.cpp`의 `EXPECT_THROW` 두 번째 인자만 갱신하면 되므로 우선순위는 **Low**로 판단한다.

## 3. `docs/screens.md`의 "[6] 출고 처리" 예시 화면 마지막 줄 "상태 CONFIRMED"는 오타로 재확인

- `docs/screens.md` 168~192행 예시 화면 마지막 줄이 "상태 CONFIRMED"로 되어 있으나, 같은 문서 192행 자체가 이미 "requirement.md 5.7절 기준으로는 RELEASE가 맞을 것으로 보인다(예시 오타로 추정)"라고 명시해 두었다.
- **확정 결과**: 이 문서(testcase.md TC-P4-040)에서도 동일하게 오타로 간주하고, 실제 구현/회귀 케이스는 반드시 `상태: RELEASE`로 표시되는 것을 정답으로 삼는다. 이미 screens.md 자체가 결론을 내려둔 사안이라 추가 확인 없이 그대로 반영했다(우선순위 **Low**, 문서 간 모순 아님 — 단일 문서 내 자기 정정).

---

## 우선순위 요약

1. **(Medium) 1번** — `ReleaseService`의 `ISampleRepository` 의존 여부(타입 시스템 차단 vs Mock 런타임 검증). Develope 구현 시 결정, 결정 즉시 이 문서와 테스트 갱신.
2. **(Low) 2번** — 예외 타입 통일(`std::invalid_argument` 가정). Develope 구현 시그니처에 맞춰 사소하게 조정 가능.
3. **(Low) 3번** — screens.md 예시의 "상태 CONFIRMED" 오타. 이미 문서 자체가 정정 방향을 명시해 사용자 확인 불필요.

모든 항목이 Develope의 구현 재량으로 해소 가능한 범위이며, Phase 4 착수를 막는 High 우선순위 미결 사항은 없다.
