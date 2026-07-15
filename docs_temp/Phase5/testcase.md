# Phase 5 테스트케이스 — 모니터링 (상태별 주문 현황 / 재고 현황)

범위: `docs/PLAN.md` Phase 5 = `MonitorService` + `[4] 모니터링` 메뉴(`MainController` 하위 라우팅).
관련 요구사항: `docs/requirement.md` 5.5절(모니터링), 5.1절(메인 메뉴 요약 정보), 4절(주문 상태 정의).
관련 설계: `docs/design.md` §3(패키지 구조 — `Service/MonitorService`), §6-5(모니터링 로직 배치), §8("대기 수요"는 RESERVED 합 기준으로 정의 — 이미 확정), §11.1(유닛 테스트 대상).
관련 화면: `docs/screens.md` 메인 메뉴, `[4] 모니터링` 화면 예시.

## 0. 병렬 개발 전략에 따른 전제

이 브랜치(`phase/5-monitoring`)는 master(Phase 0+1만 존재) 기준으로 분기했다. Phase 2~4의 Service/State 클래스는 아직 없으므로, 테스트는 `Order`/`Sample`/`ProductionQueueEntry`를 Mock Repository에 **직접 시딩**해 다양한 상태 조합을 재현한다(Phase 2~4 실제 전이 로직을 거치지 않음). `MonitorService`는 순수 조회/집계 로직만 담당하므로 이 전략으로 완전히 독립 검증 가능하다(design.md §6-5: "모니터링 화면은 저장된 값을 그대로 읽기만 한다").

- `Model/Order.h`, `Model/Sample.h`, `Model/ProductionQueueEntry.h`, `Repository/IOrderRepository.h`, `ISampleRepository.h`, `IProductionQueueRepository.h`: Phase 0에 이미 존재, 그대로 사용.
- `Service/MonitorService`: 아직 미구현(Phase 5에서 Develope가 신규 작성) — 아래 가정 시그니처로 테스트를 작성했다. 실제 시그니처가 다르면 Develope 구현 후 tester가 이 파일과 `Tests/MonitorServiceTest.cpp`를 갱신한다.

```cpp
enum class InventoryLevel { SUFFICIENT, SHORTAGE, DEPLETED }; // 여유 / 부족 / 고갈

struct SampleInventoryStatus {
    std::string sampleId;
    std::string sampleName;
    int stock;
    int pendingDemand;      // 해당 시료에 대한 RESERVED 주문 수량 합 (design.md §8 확정)
    InventoryLevel level;
};

struct MonitoringSummary {
    int sampleCount;           // 등록 시료 종수
    int totalStock;            // 총 재고 합계 (모든 시료 stock 합)
    int totalOrderCount;       // 전체 주문 건수
    int productionQueueCount;  // 생산라인 대기 건수
};

class MonitorService {
public:
    MonitorService(IOrderRepository& orderRepo, ISampleRepository& sampleRepo,
                   IProductionQueueRepository& queueRepo);

    // REJECTED 제외, 상태별 목록 (requirement.md 5.5절 "주문량 확인")
    std::vector<Order> ordersByStatus(OrderStatus status) const;

    // RESERVED/CONFIRMED/PRODUCING/RELEASE 4개 상태의 건수 맵 (REJECTED 키 없음)
    std::map<OrderStatus, int> countAllStatuses() const;

    // 시료별 재고 현황 (requirement.md 5.5절 "재고량 확인")
    std::vector<SampleInventoryStatus> inventoryStatus() const;

    // 메인 메뉴 상단 요약 정보 (requirement.md 5.1절, screens.md 메인 메뉴)
    MonitoringSummary summary() const;
};
```

---

## A. 상태별 주문 현황 (`MonitorService::countAllStatuses` / `ordersByStatus`)

### TC-P5-001: 5개 상태가 섞여 있을 때 REJECTED를 제외한 4개 상태만 집계
- **Given**: `IOrderRepository`에 RESERVED 2건, CONFIRMED 1건, PRODUCING 3건, RELEASE 1건, REJECTED 2건이 섞여 저장되어 있음.
- **When**: `countAllStatuses()`를 호출한다.
- **Then**: 반환된 맵은 `{RESERVED:2, CONFIRMED:1, PRODUCING:3, RELEASE:1}` 4개 키만 갖고, `REJECTED` 키는 존재하지 않는다(0건으로라도 포함되지 않음).
- **참조**: requirement.md 5.5절("REJECTED는 유효한 주문이 아니므로 모니터링에서 제외"), 4절(REJECTED 정의)

### TC-P5-002: `ordersByStatus(REJECTED)`는 명시적으로 빈 목록을 반환한다
- **Given**: REJECTED 주문 2건을 포함해 여러 상태의 주문이 저장되어 있음.
- **When**: `ordersByStatus(OrderStatus::REJECTED)`를 호출한다.
- **Then**: 빈 목록을 반환한다(REJECTED 전용 조회 자체를 막을지, 빈 목록으로 응답할지는 설계 선택이나, 최소한 REJECTED 주문이 결과에 섞여 나오면 안 된다는 점은 확정. 이 문서는 "빈 목록 반환"을 기본 가정으로 채택 — abnormal.md 참고).
- **참조**: requirement.md 5.5절

### TC-P5-003: 특정 상태로 조회 시 해당 상태 주문만 정확히 반환
- **Given**: RESERVED 2건(`ORD-001`, `ORD-002`), PRODUCING 1건(`ORD-003`)이 저장되어 있음.
- **When**: `ordersByStatus(OrderStatus::RESERVED)`를 호출한다.
- **Then**: `ORD-001`, `ORD-002` 2건만 반환하고 `ORD-003`은 포함되지 않는다.

### TC-P5-004: 특정 상태의 주문이 하나도 없으면 빈 목록/0건
- **Given**: 저장소에 RESERVED, CONFIRMED 주문만 있고 PRODUCING 주문은 없음.
- **When**: `countAllStatuses()`와 `ordersByStatus(OrderStatus::PRODUCING)`를 호출한다.
- **Then**: `countAllStatuses()`의 `PRODUCING` 값은 0(맵에 키가 있다면 0, 아래 TC-P5-001과 일관되게 "0건도 존재하는 키로 포함"할지 "키 자체를 생략"할지는 구현 선택 — 이 문서는 "0건이면 키 자체가 없거나 값이 0"인 두 경우 모두 통과하도록 테스트를 작성한다). `ordersByStatus(PRODUCING)`은 빈 목록을 반환한다.

### TC-P5-005: 화면의 "PRODUCING ↔ 생산라인 대기" 표기는 서로 다른 저장소에서 독립적으로 집계된다 (설계 의도 확인)
- **Given**: `IOrderRepository`에 PRODUCING 주문 3건, `IProductionQueueRepository`에 큐 엔트리 2건만 저장되어 있음(의도적으로 불일치 — 예: 테스트 데이터 정합성이 깨진 상황을 가정).
- **When**: `countAllStatuses()`의 `PRODUCING` 값과 `summary().productionQueueCount`를 각각 확인한다.
- **Then**: `PRODUCING` 카운트는 3(주문 저장소 기준), `productionQueueCount`는 2(큐 저장소 기준)로 **서로 다른 값이 그대로 반환**된다 — `MonitorService`가 두 값을 교차 검증(cross-validate)하지 않고 각자의 Repository를 있는 그대로 신뢰함을 확인한다(design.md §4.2 Repository/DIP 원칙: Service는 저장된 값을 그대로 읽기만 함).
- **참조**: screens.md `[4] 모니터링` 화면의 "PRODUCING 3건 <- 생산라인 대기" 주석은 정상 상태에서 두 값이 개념적으로 일치함을 보여주는 것일 뿐, `MonitorService`가 한쪽 값에서 다른 쪽을 유도하는 것은 아님(정상 운영 중에는 PRODUCING 주문 1건 = 큐 엔트리 1건이 1:1 대응하지만, 이는 Phase 3 `ProductionService`가 지켜야 할 불변식이지 `MonitorService`의 책임이 아니다 — abnormal.md 참고).

---

## B. 재고 현황 (`MonitorService::inventoryStatus`) — "대기 수요" 정의 포함

### TC-P5-010: 재고가 대기 수요(RESERVED 합)보다 많으면 "여유"
- **Given**: 시료 `S-001`(stock=10). `S-001`에 대한 RESERVED 주문 2건(수량 3, 2 = 합계 5)이 있음.
- **When**: `inventoryStatus()`를 호출한다.
- **Then**: `S-001`의 `pendingDemand`는 5, `level`은 `SUFFICIENT`(여유)다(`stock(10) >= pendingDemand(5)`).
- **참조**: requirement.md 5.5절("여유: 주문 대비 재고 충분")

### TC-P5-011: 재고가 대기 수요보다 적으면(0은 아님) "부족"
- **Given**: 시료 `S-002`(stock=3). `S-002`에 대한 RESERVED 주문 1건(수량 10)이 있음.
- **When**: `inventoryStatus()`를 호출한다.
- **Then**: `pendingDemand`는 10, `level`은 `SHORTAGE`(부족)다(`0 < stock(3) < pendingDemand(10)`).
- **참조**: requirement.md 5.5절("부족: 주문 대비 재고 수량 부족")

### TC-P5-012: 재고가 정확히 0이면 대기 수요와 무관하게 "고갈"
- **Given**: 시료 `S-003`(stock=0). `S-003`에 대한 RESERVED 주문이 **하나도 없음**(대기 수요=0).
- **When**: `inventoryStatus()`를 호출한다.
- **Then**: `level`은 `DEPLETED`(고갈)다 — 요구사항이 "고갈: 수량이 0인 상태"를 재고량 자체만으로 정의하므로, 대기 수요가 0이라 해도(즉 아무도 기다리지 않아도) 재고가 0이면 무조건 고갈로 분류한다(부족/여유 판정보다 우선 적용).
- **참조**: requirement.md 5.5절("고갈: 수량이 0인 상태" — 대기 수요 비교 조건이 없는 유일한 규칙)

### TC-P5-013: 재고와 대기 수요가 정확히 같으면 "여유" (경계값, 가정)
- **Given**: 시료 `S-004`(stock=7). `S-004`에 대한 RESERVED 주문 합계가 정확히 7.
- **When**: `inventoryStatus()`를 호출한다.
- **Then**: `level`은 `SUFFICIENT`(여유)다 — "충분"을 "대기 수요를 전량 충당 가능"으로 해석해 `stock == pendingDemand`도 여유에 포함시킨다(경계값 처리 방침은 요구사항에 명시가 없어 이 문서에서 확정 채택 — abnormal.md 1번 항목 참고).

### TC-P5-014 (design.md §8 핵심 확정 사항): CONFIRMED/PRODUCING 주문 수량은 대기 수요에서 제외된다
- **Given**: 시료 `S-005`(stock=5). `S-005`에 대해 RESERVED 주문 1건(수량 2), CONFIRMED 주문 1건(수량 100), PRODUCING 주문 1건(수량 200)이 있음.
- **When**: `inventoryStatus()`를 호출한다.
- **Then**: `S-005`의 `pendingDemand`는 **2**(RESERVED 수량 합만, CONFIRMED/PRODUCING 수량은 절대 합산하지 않음)이고, `level`은 `SUFFICIENT`(여유, `stock(5) >= pendingDemand(2)`)다. CONFIRMED/PRODUCING 주문은 이미 승인 시점에 재고가 반영(승인 즉시 차감 또는 생산 큐 등록)되었으므로 "재고 대비 앞으로 필요한 수요"에 다시 포함시키면 이중 계산이 된다.
- **이 케이스가 중요한 이유**: 만약 구현이 실수로 RESERVED 외 상태까지 대기 수요에 합산하면(`pendingDemand=302`), `level`이 `SHORTAGE`로 잘못 판정된다. 이 회귀를 반드시 잡아야 한다.
- **참조**: design.md §8("승인/생산 시점에 이미 재고가 반영된 CONFIRMED/PRODUCING 주문은 대기 수요에서 제외"), CLAUDE.md 3절

### TC-P5-015: REJECTED 주문 수량도 대기 수요에 포함되지 않는다
- **Given**: 시료 `S-006`(stock=5). `S-006`에 대해 REJECTED 주문 1건(수량 50)만 있고 RESERVED 주문은 없음.
- **When**: `inventoryStatus()`를 호출한다.
- **Then**: `pendingDemand`는 0, `level`은 `SUFFICIENT`(여유)다.

### TC-P5-016: 여러 시료의 재고 현황이 각각 독립적으로 계산된다
- **Given**: `S-001`(stock=10, RESERVED 합 5 → 여유), `S-002`(stock=3, RESERVED 합 10 → 부족), `S-003`(stock=0, RESERVED 없음 → 고갈) 3종이 함께 저장되어 있음.
- **When**: `inventoryStatus()`를 호출한다.
- **Then**: 반환된 목록은 3건이며, 각 항목의 `stock`/`pendingDemand`/`level`이 서로 영향을 주지 않고 각자 정확하다(다른 시료의 주문 수량이 섞여 합산되지 않음 — `sampleId` 기준으로 필터링됨을 확인).

---

## C. 메인 메뉴 요약 정보 (`MonitorService::summary`)

### TC-P5-020: 등록 시료 종수/총 재고 합계
- **Given**: 시료 3종, stock = 10, 0, 5 (합계 15).
- **When**: `summary()`를 호출한다.
- **Then**: `sampleCount`는 3, `totalStock`은 15다.
- **참조**: screens.md 메인 메뉴("등록 시료 12종, 총 재고 2800ea")

### TC-P5-021: 전체 주문 건수는 상태와 무관하게 저장소의 모든 주문을 센다 (REJECTED 포함, 가정)
- **Given**: RESERVED 1건, CONFIRMED 1건, PRODUCING 1건, RELEASE 1건, REJECTED 1건 = 총 5건.
- **When**: `summary()`를 호출한다.
- **Then**: `totalOrderCount`는 **5**다 — 메인 메뉴 요약의 "전체 주문 N건"은 requirement.md 5.5절의 "REJECTED 제외" 규칙(주문량 확인 서브메뉴 한정)과는 별개로, 시스템에 존재하는 모든 주문 레코드 수를 그대로 보여주는 것으로 가정한다(요구사항에 명시가 없어 이 문서에서 확정 채택 — abnormal.md 2번 항목, Develope/사용자 확인 필요).

### TC-P5-022: 생산라인 대기 건수는 생산 큐 저장소의 전체 엔트리 수
- **Given**: `IProductionQueueRepository`에 엔트리 3건(그 중 0번째만 `startedAt`이 채워진 "현재 생산 중" 항목).
- **When**: `summary()`를 호출한다.
- **Then**: `productionQueueCount`는 **3**이다 — 현재 처리 중인 1건과 대기 중인 2건을 구분하지 않고 큐 전체 크기를 그대로 사용한다(생산라인 조회([5]) 화면은 이 둘을 구분해 보여주지만, 메인 메뉴 요약 수치 자체는 구분하지 않는 것으로 가정 — abnormal.md 2번 항목).
- **참조**: screens.md 메인 메뉴("생산라인 8건 대기")

### TC-P5-023: 시료/주문/생산큐가 각각 여러 건이어도 요약 값이 정확히 합산된다 (종합)
- **Given**: 시료 2종(stock 10, 20 → 합계 30), 주문 4건(각 상태 섞임), 생산 큐 2건.
- **When**: `summary()`를 호출한다.
- **Then**: `sampleCount=2`, `totalStock=30`, `totalOrderCount=4`, `productionQueueCount=2`.

---

## D. 경계값 — 전체 저장소가 비어 있을 때

### TC-P5-030: 주문/시료/생산큐가 전부 비어 있으면 크래시 없이 0/빈 목록
- **Given**: `IOrderRepository::findAll()`, `ISampleRepository::findAll()`, `IProductionQueueRepository::findAll()`이 모두 빈 벡터를 반환.
- **When**: `countAllStatuses()`, `ordersByStatus(임의 상태)`, `inventoryStatus()`, `summary()`를 각각 호출한다.
- **Then**:
  - `countAllStatuses()`: 빈 맵이거나 4개 키 모두 0(둘 다 허용, TC-P5-004와 동일 원칙).
  - `ordersByStatus(...)`: 빈 목록.
  - `inventoryStatus()`: 빈 목록.
  - `summary()`: `{sampleCount:0, totalStock:0, totalOrderCount:0, productionQueueCount:0}`.
  - 예외를 던지거나 크래시하지 않는다.
- **참조**: CLAUDE.md 작업 지시("경계: 주문/시료/생산큐가 전부 비어있을 때 크래시 없이 0/빈 목록으로 정상 표시")

### TC-P5-031: 시료는 있지만 주문이 하나도 없을 때 재고 현황은 전부 "여유"(재고>0) 또는 "고갈"(재고=0)
- **Given**: 시료 2종(stock=5, stock=0), 주문/생산큐는 비어 있음.
- **When**: `inventoryStatus()`를 호출한다.
- **Then**: `stock=5`인 시료는 `pendingDemand=0`, `level=SUFFICIENT`. `stock=0`인 시료는 `pendingDemand=0`, `level=DEPLETED`(TC-P5-012와 동일 원칙 재확인).

### TC-P5-032: 주문은 있지만 그 주문이 가리키는 시료가 저장소에 없을 때 (데이터 정합성 경계)
- **Given**: `IOrderRepository`에 `sampleId="S-999"`(존재하지 않는 시료 ID)인 RESERVED 주문이 있고, `ISampleRepository`에는 `S-999`가 없음.
- **When**: `inventoryStatus()`를 호출한다.
- **Then**: `S-999`는 `ISampleRepository::findAll()` 결과에 없으므로 `inventoryStatus()` 목록에도 나타나지 않는다(재고 현황은 등록된 시료 기준으로만 순회하며, 존재하지 않는 시료의 유령 항목을 만들지 않는다). 크래시하지 않는다.
- **참조**: OrderFactory가 생성 시점에 이미 "등록된 시료만 주문 가능"을 강제하므로(design.md §4.4) 정상 흐름에서는 발생하지 않지만, 방어적 동작을 확인하는 경계 케이스.

---

## 참조 우선순위 요약

- 이 문서에서 "가정/확정 채택"으로 표시한 항목(TC-P5-002, 013, 021, 022)은 요구사항에 명시적 규정이 없어 tester가 임의로 확정하지 않고 `abnormal.md`에 근거와 함께 기록했다. Develope 착수 시 이 문서의 가정을 기본값으로 구현하되, 최종 확정은 사용자 확인 후 진행한다.
