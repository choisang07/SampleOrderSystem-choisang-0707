# Phase 3 특이사항 기록 (요구사항/설계 모순, 모호성) [초안]

Test 에이전트는 Edit/Bash로 프로덕션 코드나 PRD/요구사항 문서를 수정하지 않고 아래 항목을 기록만 한다.
[testcase.md](testcase.md)에서 "가정"으로 표시한 분기들의 근거를 여기 모았다. Phase 1(`docs_temp/Phase1/abnormal.md`)과 달리 이번 항목들은 대부분 **`docs/design.md` 원문 자체의 서술 방식(선택지를 열어둔 표현, 시그니처와 설명 문단 간 불일치)에서 비롯**된다.

## 1. (High, 미확정) `IOrderState`의 허용되지 않는 전이(approve/reject/release) 시 동작 — no-op vs 예외

- `docs/design.md` §4.1: "`ReservedState::reject()`: `RejectedState::instance()` 반환. 다른 상태(`ConfirmedState`, `ProducingState` 등)의 `reject()`는 **아무 것도 하지 않고 자기 자신(`*this`)을 반환하거나 예외를 던져**(허용되지 않는 전이) '...'을 타입 시스템 레벨에서 강제한다." — "또는"으로 두 선택지를 모두 열어둔 채 확정하지 않았다.
- `completeProduction()`은 같은 절에서 "`ProducingState::completeProduction()`만 실제로 동작하고 **나머지 상태는 no-op**"이라고 **명확하게(선택지 없이)** 서술되어 있어, completeProduction만은 모호하지 않다.
- **테스트 작성 시 임시 가정**: approve/reject/release의 허용되지 않는 전이는 **예외(`std::logic_error`)를 던지는 쪽**으로 가정해 TC-P3-020~022를 작성했다. 이유: (1) 콘솔 메뉴에서 "이미 승인된 주문을 다시 승인 시도"는 사용자 실수(잘못된 목록 필터링 버그 등)의 신호일 가능성이 높아 조용히 무시하기보다 즉시 드러나는 편이 안전, (2) `OrderService`가 이 예외를 잡아 "이미 처리된 주문입니다" 같은 사용자 메시지로 변환하기 쉬움(no-op이면 Controller가 "성공"과 "무시됨"을 구분할 방법이 없어짐).
- **영향받는 테스트케이스**: TC-P3-020, 021, 022. Develope 구현 시 반대로(no-op) 결정되면 이 3개 케이스만 갱신하면 되고, 다른 케이스에는 영향 없다.
- **우선순위**: High — Phase 3 착수(State 클래스 구현) 전에 확정이 필요하다. 사용자 확인 요망.

## 2. `Model/Order.h`에 design.md §4.1이 서술하는 `IOrderState*` 상태 포인터 필드가 없음

- design.md §4.1: "`Order`는 현재 상태를 non-owning 참조/포인터(`IOrderState*`)로만 가리키고, `Order::status()`는 `state_->statusCode()`를 그대로 리턴..."
- 그러나 현재(Phase 0에서 이식된) `SampleOrderSystem/Model/Order.h`는 `OrderStatus status;` 필드 하나만 갖고 있고, `IOrderState*` 포인터도 `status()` 접근자 메서드도 없다(순수 데이터 구조 + JSON 매핑만, design.md §3의 "Model은 로직 없음" 원칙과 일치하는 현재 형태).
- **테스트 작성 시 임시 가정**: 이 문서의 테스트는 State 메서드 호출 후 관찰 가능한 결과, 즉 **`order.status`(현재 존재하는 필드) 값 변화**만을 검증한다 — Develope가 (a) design.md 원문대로 `Order`에 `IOrderState*` 포인터를 추가하고 `status` 필드를 제거/대체하든, (b) 각 State 메서드가 `order.status`를 직접 대입하는 더 단순한 방식을 택하든, 어느 쪽이든 이 문서의 테스트 기대값(관찰 가능한 `status` 결과)은 그대로 성립해야 한다.
- **영향받는 테스트케이스**: 전체(A~F). JSON 직렬화(`to_json`/`from_json`)는 이미 `OrderStatus status` 필드 기준으로 구현되어 있으므로(§5 "State 패턴은 순수 인메모리 구조, 직렬화 시 `statusCode()`만 문자열로 저장"과 일치하려면), (a) 방식을 택하더라도 최소한 `to_json` 직전에는 `status` 필드나 동등한 접근자가 있어야 한다는 점은 Develope가 유의해야 한다.
- **우선순위**: Medium — 테스트 자체는 어느 구현이든 통과 가능하도록 작성했으므로 Phase 3 착수를 막지는 않지만, `Order.h` 구조 변경 여부는 Develope의 설계 판단이 필요하다.

## 3. `ProductionService` 생성자 인자 — design.md §4.3(2-인자) vs 실제 필요 의존성(3-인자)

- design.md §4.3: `explicit ProductionService(IProductionQueueRepository& queueRepo, ISampleRepository& sampleRepo);` — `IOrderRepository`를 받지 않는다.
- 그러나 같은 문서 §6.2: "`completeActive`... 해당 주문이 `ProducingState::completeProduction()`을 통해 `ConfirmedState`로 전이"라고 서술한다. 큐 항목(`ProductionQueueEntry`)에는 `orderId`만 있고 `Order` 객체 자체가 없으므로, `completeActive()`가 실제로 해당 주문의 상태를 `PRODUCING → CONFIRMED`로 바꾸려면 `IOrderRepository`로 `orderId`를 조회(`findById`) → 상태 전이 → 저장(`save`)하는 과정이 필요하다. 2-인자 생성자만으로는 이 책임을 수행할 수 없다.
- **테스트 작성 시 임시 가정**: `ProductionService(IOrderRepository& orderRepo, IProductionQueueRepository& queueRepo, ISampleRepository& sampleRepo)` 3-인자 생성자로 가정하고 `MockOrderRepository`를 함께 사용해 TC-P3-034를 작성했다.
- **대안(Develope가 고려할 수 있는 다른 해법)**: `completeActive()`가 주문 상태 전이를 직접 하지 않고 "완료된 항목 목록"만 반환하도록 시그니처를 바꾼 뒤, 호출부(`MainController` 또는 `OrderService`)가 `IOrderRepository`를 이용해 상태 전이를 마무리하는 방식도 가능하다. 이 경우 `ProductionService`는 design.md §4.3 원문 그대로 2-인자를 유지할 수 있다. 이 문서는 "재고 반영 + 큐 제거"까지는 `ProductionService`가, "주문 상태 전이"는 별도 계층이 맡는 이 대안도 §4.2(Repository+DIP)/§2(단일 책임)와 상충하지 않는다고 판단하지만, 최종 선택은 Develope 몫으로 남긴다.
- **영향받는 테스트케이스**: TC-P3-034, 035(완료 처리 전체 플로우). Develope가 대안을 택하면 `MockOrderRepository` 사용 부분만 조정하면 된다.
- **우선순위**: High — `ProductionService`의 생성자 시그니처 자체를 결정하는 문제라 구현 착수 전 확인이 필요하다.

## 4. `SampleService`에 재고 조회/조정 메서드가 없음(Phase 1 범위 밖)

- Phase 1에서 구현된 `Service/SampleService.h`는 `registerSample`/`listAll`/`search`만 제공한다(재고 필드는 있지만 개별 조회/증감 메서드가 없음).
- design.md §4.1은 `IOrderState::approve`가 `SampleService&`를 받아 "현재 재고를 확인 → 충당 가능하면 즉시 차감"한다고 서술하므로, Phase 3에서 `SampleService`에 최소 2개 메서드(단건 조회, 재고 증감)가 추가로 필요하다.
- **테스트 작성 시 임시 가정**: `std::optional<Sample> SampleService::findById(const std::string&) const`와 `void SampleService::adjustStock(const std::string&, int delta)`(delta 음수 허용=차감, 대상 없으면 `std::invalid_argument`)를 가정해 테스트를 작성했다. 메서드 이름/시그니처가 다르면(예: `deductStock`/`addStock`으로 분리) Develope 구현 후 tester가 테스트 코드만 갱신하면 되고, 이 문서의 Given/When/Then 자체(관찰 가능한 재고 값)는 영향받지 않는다.
- **우선순위**: Medium — 이름 결정 문제이며 Phase 1 코드(`SampleServiceTest.cpp` 등)와 충돌 없이 순수 추가(additive)이므로 회귀 리스크는 낮다.

## 5. `ProductionService::enqueue`의 책임 범위 — design.md §4.3(리터럴 시그니처) vs §6(서술)의 불일치

- §4.3 코드 스니펫: `void enqueue(ProductionQueueEntry entry);   // queueRepo에 append, 비어 있었으면 즉시 시작 처리` — 이미 완성된 entry(즉 `requiredQuantity`/`totalProductionMinutes` 계산이 끝난 상태)를 받는 것으로 읽힌다.
- §6.2 서술: "`enqueue`: `requiredQuantity = ceil(부족분 / yield)`, `totalProductionMinutes = 평균 생산시간 × requiredQuantity` 계산 후 큐 끝에 추가" — 마치 `enqueue` 자신이 계산을 수행하는 것처럼 읽힌다.
- 두 서술이 같은 메서드에 대해 "완성된 값을 받는다" vs "직접 계산한다"로 다르게 읽혀 시그니처가 불명확하다.
- **테스트 작성 시 임시 가정**: §4.3의 리터럴 시그니처(완성된 `ProductionQueueEntry`를 받아 append + `startedAt` 처리만 담당)를 채택했다. 근거: (1) `ProductionQueueEntry`의 `requiredQuantity` 계산에는 `Sample.yield`/`avgProductionTime`이 필요한데, 이는 이미 `ReservedState::approve`가 `SampleService`를 통해 접근 가능한 정보이므로 중복해서 `ProductionService`에도 계산 책임을 부여할 필요가 없다. (2) §6.2의 "enqueue: ... 계산 후"라는 표현은 "승인→생산 등록"이라는 **흐름 전체**를 요약한 문장으로, 계산 주체를 `ProductionService::enqueue` 메서드 하나로 한정한 것이 아니라 `ReservedState::approve`+`ProductionService::enqueue` 조합을 뭉뚱그려 서술한 것으로 해석했다.
- **영향받는 테스트케이스**: TC-P3-003, 005, 006, 040~043 — 실 생산량 계산 검증을 `ReservedState::approve` 결과(큐에 전달되는 `requiredQuantity` 값)로 확인하도록 작성했다.
- **우선순위**: Low — 계산 주체가 어느 클래스든 최종 관찰값(큐에 저장되는 `requiredQuantity`)은 동일해야 하므로 테스트 자체의 유효성에는 영향이 적다. 다만 Develope의 클래스 책임 분배 판단에 참고가 필요해 기록한다.

## 6. 유닛 테스트에서 wall-clock(현실 시간) 기반 로직을 어떻게 검증했는지

- requirement.md 5.6절 규칙 3: "생산 소요 시간은 현실 시간(wall-clock)을 기준으로 하며..." — 유닛 테스트는 실제로 분 단위를 대기할 수 없다.
- **검증 방법**: `ProductionQueueEntry.startedAt`을 테스트에서 임의의 과거 시각 문자열(예: 현재 시각보다 훨씬 이전, `totalProductionMinutes`를 충분히 초과)로 직접 세팅해 "완료 시점 도달"을 재현하고(TC-P3-034), 반대로 `totalProductionMinutes`를 비현실적으로 큰 값(예: 999999분)으로 세팅해 "미도달"을 재현했다(TC-P3-033). `ProductionService`가 `startedAt` 파싱 및 `now()` 비교 로직을 내부적으로 어떻게 구현하든(문자열 포맷 "YYYY-MM-DD HH:MM:SS" 파싱 방식은 design.md §5 그대로 유지된다는 전제), 이 방식이면 실제 대기 없이 결정론적으로 두 분기를 모두 검증할 수 있다.
- **참고**: 이는 이슈라기보다 테스트 기법에 대한 기록이며, 별도 확인이 필요한 미결 항목은 아니다.

---

## 우선순위 요약

1. **(High)** 1번 — `IOrderState` 허용되지 않는 전이 시 no-op vs 예외 — 사용자 확인 필요.
2. **(High)** 3번 — `ProductionService` 생성자에 `IOrderRepository` 필요 여부(2-인자 vs 3-인자, 또는 완료 처리 책임을 다른 계층으로 분리하는 대안) — Develope 착수 전 결정 필요.
3. **(Medium)** 2번 — `Order` 모델의 `IOrderState*` 포인터 반영 여부 — 테스트 자체는 어느 쪽이든 통과하므로 착수를 막지는 않으나 설계 일관성 차원에서 결정 권고.
4. **(Medium)** 4번 — `SampleService` 재고 조회/조정 메서드 이름 — Develope 구현 재량, 회귀 리스크 낮음.
5. **(Low)** 5번 — `enqueue` 계산 책임 소재 — 관찰 가능한 결과에 영향 없음.
6. 6번은 이슈가 아니라 테스트 기법 기록.

모두 확정되지 않았으나 1, 2, 3번 외에는 테스트 코드 자체를 재작성할 필요 없이 Develope 구현 후 이름/시그니처 갱신만으로 대응 가능하다(Phase 1 선례와 동일한 패턴).
