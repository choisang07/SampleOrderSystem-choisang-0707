# Phase 3 테스트케이스 — 주문 승인/거절 + 생산 라인 [초안]

범위: `docs/PLAN.md` Phase 3 = `IOrderState` 전체 구현(승인/거절 전이) + `ProductionService`(실생산량 계산, FIFO 큐, 생산완료 처리).
관련 요구사항: `docs/requirement.md` 5.4절(주문 승인/거절), 5.6절(생산 라인, **사례1/사례2 최우선**), 4절(주문 상태 정의).
관련 설계: `docs/design.md` §3(Domain/OrderState, Service/ProductionService), §4.1(State 패턴), §4.3(ProductionService), §6(재고/생산 처리 규칙), §10(FIFO 큐/lazy 완료 체크/Stateless Singleton), §11.1(유닛 테스트 대상).
화면 예시: `docs/screens.md` "[3] 주문 승인/거절", "[5] 생산라인 조회".

> **주의 (병렬 개발)**: 이 브랜치는 Phase 2(`OrderFactory`/`OrderService`)가 아직 없는 master(Phase 0+1만 존재) 기준에서 분기했다. 따라서 아래 테스트는 `OrderService::reserve` 등을 거치지 않고 **RESERVED 상태의 `Order`를 테스트 코드에서 직접 생성해 Mock Repository에 시딩**하는 방식으로 독립 검증한다. `IOrderRepository`/`IProductionQueueRepository`/`ISampleRepository` 인터페이스 계약만 정확히 지키면, Phase 2 merge 후 rebase 시에도 그대로 유효하다.

## 0. 실제 코드 확인 결과 및 가정 시그니처 (미구현 — Develope 착수 전 가정)

`Domain/OrderState/*`, `Service/ProductionService.*`는 아직 Develope가 구현하지 않았다(design.md §3, §4.1, §4.3 기준으로 아래 시그니처를 가정). **design.md 원문과 실제 코드 사이에 몇 가지 간극이 있어 abnormal.md에 근거를 남기고 아래와 같이 가정을 확정했다** — 실제 시그니처가 다르면 Develope 구현 후 tester가 이 문서와 `Tests/*.cpp`를 갱신한다.

```cpp
// Domain/OrderState/IOrderState.h (design.md §4.1 그대로)
class IOrderState {
public:
    virtual ~IOrderState() = default;
    virtual IOrderState& approve(Order& order, SampleService& sampleService, ProductionService& productionService) const = 0;
    virtual IOrderState& reject(Order& order) const = 0;
    virtual IOrderState& completeProduction(Order& order, SampleService& sampleService) const = 0;
    virtual IOrderState& release(Order& order) const = 0;
    virtual OrderStatus statusCode() const = 0;
};
// ReservedState / ProducingState / ConfirmedState / ReleasedState / RejectedState
// 각각 static IOrderState& instance(); 를 제공하는 stateless singleton (design.md §10).
// 허용되지 않는 전이(예: ConfirmedState에서 approve/reject, RejectedState에서 reject 등)는
// std::logic_error를 던진다 (가정 — abnormal.md 1번 항목, 미확정 상태로 사용자 확인 필요).
// completeProduction()은 ProducingState에서만 실제 동작하고, 그 외 상태는 아무 것도 하지 않고
// *this(자기 자신)를 반환한다 (design.md §4.1 원문에 "no-op"으로 명시되어 있어 모호함 없음).

// Service/SampleService.h — Phase 1 기존 메서드 + Phase 3 확장분 (abnormal.md 4번 항목 가정)
class SampleService {
public:
    // ...(Phase 1 기존: registerSample/listAll/search)
    std::optional<Sample> findById(const std::string& id) const;      // 신규 가정
    void adjustStock(const std::string& id, int delta);                // 신규 가정. delta<0(차감)/>0(가산) 허용, 대상 없으면 std::invalid_argument
};

// Service/ProductionService.h (design.md §4.3 리터럴 시그니처 + IOrderRepository 의존성 추가 가정 — abnormal.md 3번 항목)
class ProductionService {
public:
    ProductionService(IOrderRepository& orderRepo, IProductionQueueRepository& queueRepo, ISampleRepository& sampleRepo);
    void enqueue(const ProductionQueueEntry& entry);              // 큐 끝에 append. 큐가 비어 있었으면 startedAt=현재 wall-clock 채움("즉시 시작")
    std::optional<ProductionQueueEntry> peekActive() const;       // findAll()[0] (큐 head)
    void completeActive();                                        // head의 startedAt+totalProductionMinutes가 현재 시각을 지났으면 완료 처리
    // requiredQuantity(=ceil(부족분/yield))/totalProductionMinutes 계산 자체는 ReservedState::approve가
    // SampleService로 얻은 sample.yield/avgProductionTime을 이용해 계산 후 enqueue에 넘기는 것으로 가정
    // (abnormal.md 5번 항목 — design.md §4.3과 §6 서술의 불일치 해소를 위한 선택).
};
```

- `Model/Order.h`(Phase 0)는 현재 `OrderStatus status` 필드만 갖고, design.md §4.1이 서술하는 "Order가 `IOrderState*`를 non-owning으로 들고 있다"는 구조는 아직 반영되어 있지 않다(abnormal.md 2번 항목). 이 문서의 테스트는 **State 메서드 호출 후 `order.status`가 기대한 `OrderStatus`로 바뀌었는지**만 관찰 가능한 계약으로 검증한다 — Develope가 `Order`에 `IOrderState*` 포인터를 추가하든, State 메서드가 `order.status`를 직접 대입하든 이 계약(관찰 가능한 결과)은 동일하게 성립해야 한다.

---

## A. 주문 승인 — 재고로 전량 충당 (`ReservedState::approve`)

### TC-P3-001: 재고가 주문 수량 이상이면 즉시 CONFIRMED
- **Given**: `S-001`(수율 0.9, 재고 200)이 등록되어 있음. `ORD-1`(sampleId=S-001, quantity=150, status=RESERVED)이 접수되어 있음.
- **When**: 승인 처리(`ReservedState::instance().approve(order, sampleService, productionService)`)를 실행한다.
- **Then**: 재고에서 즉시 150 차감(200→50), 생산 큐에 아무 것도 추가되지 않음(`queueRepo.save` 호출 없음), `order.status == CONFIRMED`로 전이한다.
- **참조**: requirement.md 5.4절, design.md §6.1

### TC-P3-002: 재고와 주문 수량이 정확히 일치(경계값)
- **Given**: `S-001`(재고 100), `ORD-1`(quantity=100).
- **When**: 승인 처리.
- **Then**: 재고 100→0으로 차감, 부족분 0이므로 생산 큐 미사용, `CONFIRMED` 전이.
- **참조**: design.md §6.1("min(stock, quantity)")

## B. 주문 승인 — 재고 부족(생산 필요) (`ReservedState::approve` + `ProductionService::enqueue`)

### TC-P3-003: 재고 0, 전량 부족 (requirement.md 사례1 전반부)
- **대상 기능**: 실 생산량 계산과 즉시 재고 반영 없음(부족분 전량 생산)
- **Given**: `S-001`(수율 0.5, 재고 0)이 등록되어 있음. `ORD-1`(sampleId=S-001, quantity=50, RESERVED).
- **When**: 승인 처리.
- **Then**: 부족분 = 50 - min(0,50) = 50. 재고는 이미 0이라 추가 차감 없음. 실 생산량 = `ceil(50/0.5) = 100`. `ProductionService::enqueue`가 `requiredQuantity=100`, `totalProductionMinutes = avgProductionTime × 100`인 `ProductionQueueEntry(orderId="ORD-1", sampleId="S-001")`로 호출된다. `order.status == PRODUCING`으로 전이한다.
- **인용 근거**: requirement.md 5.6절 사례1 — "부족분 50 → 실 생산량 = ceil(50 / 0.5) = 100개 생산 진행".
- **참조**: requirement.md 5.6절 사례1, design.md §6.1~6.2

### TC-P3-004: 사례1 후반부 — 생산 완료 후 재고 반영과 잔여 재고로 후속 주문 충당
- **대상 기능**: 생산 완료 시점 일괄 재고 반영(수율 재적용 없음) + 잔여 재고로 이후 주문 즉시 충당
- **Given**: TC-P3-003 완료 직후 상태(큐에 `requiredQuantity=100`인 항목 1건, `startedAt`이 이미 충분히 지난 과거 시각으로 설정되어 "완료 시점 도달"을 재현). `S-001` 재고는 여전히 0.
- **When**: (1) `ProductionService::completeActive()` 호출 → (2) 이어서 동일 시료로 수량 50인 새 주문 `ORD-2`(RESERVED)를 승인 처리.
- **Then**: (1) 완료 처리 시 `requiredQuantity=100` 전량이 **수율 재적용 없이** `S-001.stock`에 반영되어 0→100이 되고(규칙 4), `ORD-1.status`가 `PRODUCING → CONFIRMED`로 전이하며, 큐에서 제거된다. (2) `ORD-2` 승인 시 현재 재고 100 ≥ 주문 50이므로 **생산 없이 즉시 충당**되어 재고 100→50, `ORD-2.status == CONFIRMED`.
- **인용 근거**: requirement.md 5.6절 사례1 — "생산 완료 시 재고 반영(0 + 100 = 100)... 이후 동일 제품 50개 주문이 다시 들어오면, 재고 50개로 즉시 충당 가능(생산 불필요)". *(원문은 "재고 50개로 충당"이라 표현했으나 실제 수치 흐름상 재고 100개 중 50개로 충당되는 것을 의미 — 문서 표현이 축약된 것으로 해석함, 계산 결과는 requirement.md 서술과 정합)*
- **참조**: requirement.md 5.6절 사례1, design.md §6.2(수율 재적용 없음), §10(lazy 완료 체크)

### TC-P3-005: 사례2 — 승인 즉시 기존 재고 전량 소진 후 부족분만 생산
- **Given**: `S-001`(수율 0.5, 재고 50)이 등록되어 있음. `ORD-2`(sampleId=S-001, quantity=100, RESERVED).
- **When**: 승인 처리.
- **Then**: 재고로 충당 가능한 만큼(min(50,100)=50)을 **승인 즉시** 차감(재고 50→0). 남은 부족분 = 100-50 = 50. 실 생산량 = `ceil(50/0.5) = 100`. `enqueue(requiredQuantity=100, ...)` 호출, `order.status == PRODUCING`.
- **인용 근거**: requirement.md 5.6절 사례2 — "주문2: 100개 주문 접수 → 승인 즉시 기존 재고 50개 전량을 차감(재고 50 → 0)... 남은 부족분 50 → 실 생산량 = ceil(50 / 0.5) = 100개 생산 진행(PRODUCING)".
- **참조**: requirement.md 5.6절 사례2, design.md §6.1

### TC-P3-006: 사례2 — 생산 큐를 무시하고 현재 재고 수치만으로 부족분을 재계산 (최우선 검증 대상)
- **대상 기능**: "재고 확인 시 생산 큐는 절대 고려하지 않는다"는 규칙의 직접 검증
- **Given**: TC-P3-005 직후 상태(`S-001.stock == 0`, 생산 큐에 `ORD-2`용 항목 1건이 대기/진행 중, 아직 완료 전). 이 시점에 `ORD-3`(sampleId=S-001, quantity=50, RESERVED)이 접수된다("주문2의 생산 완료 이전 시점").
- **When**: `ORD-3`을 승인 처리한다.
- **Then**: 승인 로직은 `S-001.stock`의 **현재 수치(0)만** 참조하고 생산 큐(대기 중인 `ORD-2`분)는 조회·고려하지 않는다. 부족분 = 50 - min(0,50) = 50. 실 생산량 = `ceil(50/0.5) = 100`으로 **`ORD-2`와 별개로 추가 생산**이 큐에 등록된다(`enqueue`가 `requiredQuantity=100`인 두 번째 항목으로 호출됨, orderId="ORD-3"). `ORD-3.status == PRODUCING`. 이 시점 큐에는 `ORD-2`(head), `ORD-3`(대기) 2건이 FIFO 순서로 존재한다(TC-P3-010과 연계).
- **인용 근거**: requirement.md 5.6절 사례2 — "이 시점 실제 재고는 이미 0... '재고 50개'는 최초 여유분 기준일 뿐... 재고 확인 시 생산 큐(주문2가 생산 완료 후 채울 예정 수량)는 고려하지 않으므로, 주문3도 부족분 50 → 실 생산량 = ceil(50/0.5) = 100개를 추가로 생산해야 한다... 이는 의도된 동작이다(생산 큐를 참조해 회피하지 않음)."
- **참조**: requirement.md 5.6절 사례2, design.md §6.1("재고 확인 시 productionQueue는 절대 조회하지 않는다")

---

## C. 주문 거절 (`ReservedState::reject`)

### TC-P3-010: RESERVED 주문 정상 거절
- **Given**: `ORD-1`(RESERVED).
- **When**: `ReservedState::instance().reject(order)` 호출.
- **Then**: `order.status == REJECTED`로 전이. 재고(`Sample.stock`)에는 어떤 변화도 없다(`sampleRepo.save`/`adjustStock` 호출 없음). 생산 큐에도 영향 없음.
- **참조**: requirement.md 5.4절("주문 거절: 즉시 REJECTED"), design.md §6.3

---

## D. 잘못된 상태 전이 (허용되지 않는 승인/거절/출고 시도)

> **미확정 항목**: design.md §4.1은 "다른 상태의 reject()는 아무 것도 하지 않고 자기 자신을 반환하거나 예외를 던져"라고 두 가지 선택지를 모두 열어둔 채 서술한다. 이 문서는 **예외(`std::logic_error`)를 던지는 쪽을 임시 가정**으로 채택해 테스트를 작성했다(abnormal.md 1번 항목 — Develope 착수 전 사용자 확인 필요, High 우선순위).

### TC-P3-020: 이미 CONFIRMED인 주문에 승인 재시도
- **Given**: `ORD-1`(status=CONFIRMED).
- **When**: `ConfirmedState::instance().approve(order, sampleService, productionService)` 호출.
- **Then**: (가정) `std::logic_error` 예외 발생. `order.status`는 `CONFIRMED`로 변화 없음. 재고/생산 큐 부수효과 없음.
- **참조**: design.md §4.1, abnormal.md 1번

### TC-P3-021: 이미 PRODUCING인 주문에 승인/거절 재시도
- **Given**: `ORD-1`(status=PRODUCING).
- **When**: (a) `ProducingState::instance().approve(...)`, (b) `ProducingState::instance().reject(order)`를 각각 호출.
- **Then**: (가정) 두 경우 모두 `std::logic_error`. 상태 변화 없음("생산 중 상태에서는 생산완료 처리만 의미 있다"는 설계 의도와 일치).
- **참조**: design.md §4.1("ProducingState::completeProduction()만 실제로 동작")

### TC-P3-022: 이미 REJECTED인 주문에 거절 재시도
- **Given**: `ORD-1`(status=REJECTED).
- **When**: `RejectedState::instance().reject(order)` 호출.
- **Then**: (가정) `std::logic_error`. 상태 변화 없음("RESERVED 상태에서만 거절 가능"이라는 규칙의 타입 시스템 강제).
- **참조**: design.md §4.1

### TC-P3-023: PRODUCING/CONFIRMED/REJECTED/RELEASE 상태에서 `completeProduction()` 호출 시 no-op (예외 아님, 확정)
- **Given**: `ORD-1`(status=CONFIRMED, 또는 REJECTED, RELEASE 각각).
- **When**: 해당 상태 싱글턴의 `completeProduction(order, sampleService)`를 호출한다.
- **Then**: design.md §4.1에 "ProducingState만 실제 동작, 나머지는 no-op"이라고 **명시적으로(모호함 없이)** 기술되어 있으므로, 예외를 던지지 않고 아무 부수효과 없이 자기 자신(현재 상태)을 반환한다. `order.status`는 변화 없음.
- **참조**: design.md §4.1(명시적 no-op 서술)

---

## E. FIFO 생산 큐 (`ProductionService`)

### TC-P3-030: 큐가 비어 있을 때 첫 등록은 즉시 시작(`startedAt` 채움)
- **Given**: 생산 큐가 비어 있음(`queueRepo.findAll()` → `{}`).
- **When**: `productionService.enqueue(entry)`를 호출한다(entry.startedAt은 호출 전 비어 있음("")).
- **Then**: `queueRepo.save`가 호출되며, 저장되는 entry의 `startedAt`이 현재 wall-clock 시각 문자열로 채워진다(빈 문자열이 아님) — "즉시 생산 시작" 처리.
- **참조**: design.md §4.3, §6.2

### TC-P3-031: 큐에 항목이 있을 때 신규 등록은 대기(`startedAt` 비움 유지)
- **Given**: 생산 큐에 이미 진행 중인 항목 1건(`startedAt` 채워짐)이 있음.
- **When**: 두 번째 `enqueue(entry2)` 호출.
- **Then**: `entry2`는 큐 **끝**에 추가되고 `startedAt`은 빈 문자열("")로 유지된다(대기 상태). `peekActive()`는 여전히 첫 번째 항목을 반환한다.
- **참조**: design.md §5("배열 순서 자체가 FIFO 순서, 0번째만 startedAt 채워짐"), §10

### TC-P3-032: 여러 건이 쌓였을 때 처리 순서 보장(FIFO)
- **Given**: `ORD-A`, `ORD-B`, `ORD-C` 순서로 3건을 연속 `enqueue`한다.
- **When**: `peekActive()`를 호출한다.
- **Then**: 항상 `ORD-A`(가장 먼저 등록된 항목)가 반환된다. `ORD-A`가 `completeActive()`로 제거된 이후에는 `ORD-B`가 head가 된다(등록 순서가 곧 처리 순서).
- **참조**: requirement.md 5.6절("스케줄링 전략: FIFO"), design.md §10

### TC-P3-033: 생산 완료 시점 미도달 — `completeActive()`는 아무 것도 하지 않음
- **Given**: 큐 head의 `startedAt`이 현재 시각 기준이고 `totalProductionMinutes`가 매우 큰 값(예: 999999분)이라 완료 시점에 한참 못 미침.
- **When**: `completeActive()` 호출.
- **Then**: `queueRepo.remove`/`sampleRepo`/`orderRepo.save` 어느 것도 호출되지 않는다(부수효과 없음). 큐 상태 그대로 유지.
- **참조**: design.md §10("lazy 체크")

### TC-P3-034: 생산 완료 시점 도달 — 재고 반영/주문 전이/큐 제거/다음 항목 시작
- **Given**: 큐 head(`ORD-1`, sampleId=S-001, requiredQuantity=100)의 `startedAt`이 `totalProductionMinutes`를 충분히 초과한 과거 시각. 큐에는 대기 중인 `ORD-2` 항목도 존재(2건). `ORD-1`에 대응하는 `Order`(status=PRODUCING)가 orderRepo에 존재. `S-001.stock`은 0.
- **When**: `completeActive()` 호출.
- **Then**: (1) `requiredQuantity=100` 전량이 수율 재적용 없이 `S-001.stock`에 반영(0→100). (2) `ORD-1.status`가 `PRODUCING → CONFIRMED`로 전이(orderRepo.save 호출). (3) 큐에서 `ORD-1` 항목이 제거된다(`queueRepo.remove("ORD-1")`). (4) 다음 항목 `ORD-2`가 새 head가 되고 `startedAt`이 채워져 "생산 시작" 상태가 된다.
- **참조**: requirement.md 5.6절 규칙 1, 2, 4, design.md §6.2

### TC-P3-035: 완료 처리 후 큐가 비었을 때
- **Given**: 큐에 항목이 1건뿐이고 완료 시점 도달.
- **When**: `completeActive()` 호출.
- **Then**: 해당 항목이 정상 완료 처리되어 제거되고, 큐는 빈 상태가 된다(다음 항목 없음, 예외 없이 정상 종료). `peekActive()`가 `std::nullopt`을 반환.
- **참조**: design.md §4.3

---

## F. 실 생산량(`ceil(부족분/수율)`) 계산 경계값

### TC-P3-040: 수율 1.0(무손실) — 실 생산량 = 부족분 그대로
- **Given**: `S-001`(수율 1.0, 재고 0), `ORD-1`(quantity=30).
- **When**: 승인 처리.
- **Then**: 부족분 30, 실 생산량 = `ceil(30/1.0) = 30`(부족분과 동일, 손실 없음).
- **참조**: requirement.md 5.6절, TC-P1-006(수율 상한 경계와 연계)

### TC-P3-041: 수율이 매우 낮은 값 — 큰 배율로 생산량 증폭
- **Given**: `S-001`(수율 0.01, 재고 0), `ORD-1`(quantity=3).
- **When**: 승인 처리.
- **Then**: 부족분 3, 실 생산량 = `ceil(3/0.01) = ceil(300) = 300`.
- **참조**: requirement.md 5.6절

### TC-P3-042: `ceil` 반올림 경계 — 나누어떨어지지 않는 경우
- **Given**: `S-001`(수율 0.92, 재고 30), `ORD-1`(quantity=80).
- **When**: 승인 처리.
- **Then**: 재고로 min(30,80)=30 즉시 차감(재고 30→0). 부족분 = 80-30 = 50. 실 생산량 = `ceil(50/0.92) = ceil(54.347...) = 55`(정수 올림). **주의**: `docs/screens.md` "[5] 생산라인 조회" 예시 화면의 "실생산량 54 ea"는 반올림 오차/오타로 추정되며(문서 자체가 "실제 정확한 계산값은 55"라고 명시), 이 테스트는 화면 예시의 54가 아니라 정확한 `ceil` 계산값 55를 기대값으로 사용한다.
- **참조**: docs/screens.md "[5] 생산라인 조회" 및 그 하단 "참고: 생산라인 조회 화면의 계산 예시" 절

### TC-P3-043: 나누어떨어지는 경우 — `ceil` 결과가 정수 나눗셈과 같음
- **Given**: `S-001`(수율 0.5, 재고 0), `ORD-1`(quantity=50).
- **When**: 승인 처리.
- **Then**: 실 생산량 = `ceil(50/0.5) = 100`(나머지 없이 정확히 나누어떨어짐 — 사례1과 동일한 계산 재확인).
- **참조**: requirement.md 5.6절 사례1

---

## G. Repository/Service 계층 유닛 테스트 커버리지 매핑

`Tests/OrderStateTest.cpp`(State 전이), `Tests/ProductionServiceTest.cpp`(FIFO 큐/완료 처리)로 작성했다. Mock: `Tests/Mocks/MockOrderRepository.h`(신규), `Tests/Mocks/MockProductionQueueRepository.h`(신규), `Tests/Mocks/MockSampleRepository.h`(Phase 1 기존 재사용).

| TestCase | 유닛 테스트 대상 | Mock 설정 포인트 |
|---|---|---|
| TC-P3-001, 002 | `ReservedState::approve` 재고 충분 경로 | `MockSampleRepository::findById` 스텁, `adjustStock` 호출 검증(`EXPECT_CALL`), 큐 `save` 미호출(`Times(0)`) 검증 |
| TC-P3-003, 005 (사례1/2 전반부) | `ReservedState::approve` 재고 부족 경로 + `ProductionService::enqueue` 연동 | `MockProductionQueueRepository::save`가 기대한 `requiredQuantity`/`totalProductionMinutes`로 호출되는지 인자 매처(`Field`/커스텀 matcher)로 검증 |
| TC-P3-004 (사례1 후반부) | `completeActive` 재고 반영 + 잔여 재고로 신규 주문 즉시 충당 연쇄 | `MockSampleRepository` 상태를 테스트 내에서 직접 갱신(스텁 재설정)해 "현재 재고" 변화를 시뮬레이션 |
| TC-P3-006 (사례2 핵심) | "재고 확인 시 생산 큐 무시" | `MockSampleRepository::findById`가 오직 현재 stock(0)만 반환하도록 스텁하고, 큐 관련 Mock 호출 여부와 무관하게 결과(부족분 50, 실생산량 100)가 산출되는지 확인 |
| TC-P3-010 | `ReservedState::reject` | `MockSampleRepository`/`MockProductionQueueRepository`에 어떤 메서드도 호출되지 않음(`EXPECT_CALL(...).Times(0)`) 검증 |
| TC-P3-020~022 | 잘못된 상태 전이 시 예외(가정) | `EXPECT_THROW(..., std::logic_error)` |
| TC-P3-023 | `completeProduction` no-op(확정) | 호출 후 `order.status` 불변, Mock 호출 없음 검증 |
| TC-P3-030~035 | `ProductionService` FIFO enqueue/peekActive/completeActive | `MockProductionQueueRepository::findAll`을 벡터 순서로 스텁, `startedAt` 유무로 head/대기 구분 |
| TC-P3-040~043 | 실 생산량 `ceil` 계산 경계값 | `MockSampleRepository::findById`로 다양한 `yield` 스텁, `enqueue` 호출 인자의 `requiredQuantity` 값 검증 |

시스템 테스트(`/system-test`) 이관 후보: TC-P3-001(재고 충분 즉시 승인), TC-P3-003/005(재고 부족 승인→생산), TC-P3-010(거절), TC-P3-034(생산완료→CONFIRMED 전이) — Develope 구현 완료 후 실제 화면 문구를 채취해 `run.ps1`에 추가할 것을 권고한다(Phase 1 선례와 동일 절차).
