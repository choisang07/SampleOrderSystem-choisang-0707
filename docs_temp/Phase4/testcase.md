# Phase 4 테스트케이스 — 출고 처리 (CONFIRMED → RELEASE)

범위: `docs/PLAN.md` Phase 4 = `ReleaseService` + `ConfirmedState::release`.
관련 요구사항: `docs/requirement.md` 5.7절(출고 처리), 5.6절(재고/생산 처리 규칙 — 재고는 승인 시점에만 반영), 4절(주문 상태 흐름).
관련 설계: `docs/design.md` §3(Service/ReleaseService), §4.1(State 패턴 — `IOrderState::release(Order&)`), §5(JSON 스키마), §11.1(유닛 테스트 대상).
관련 화면: `docs/screens.md` "[6] 출고 처리".

> **병렬 개발 전제**: 이 브랜치(`phase/4-release`)는 master(Phase 0+1만 존재) 기준으로 분기했다. Phase 2(`OrderService`)/Phase 3(`ProductionService`, `IOrderState`/`ConfirmedState` 등 State 패턴 구현체)가 아직 이 브랜치에 없다. 따라서 아래 테스트는 Phase 2/3 서비스를 호출하지 않고, **CONFIRMED 상태의 `Order` 객체를 테스트 코드에서 직접 만들어 Mock `IOrderRepository`에 시딩**하는 방식으로 `ReleaseService`만 독립적으로 검증한다. `Order.status`는 현재(Phase 0) 코드 기준 순수 enum 필드(`Model/Order.h`)이며, State 클래스는 아직 존재하지 않으므로 상태 전이는 `ReleaseService` 내부에서 `order.status` 필드를 직접 확인/변경하는 것으로 가정한다(Phase 3 merge/rebase 후 실제로는 `IOrderState`를 통한 전이로 대체될 수 있으나, 계약(CONFIRMED만 허용 → RELEASE로 전이, 그 외 상태는 거부)은 동일하다).

## 0. Phase 0 이식 코드 확인 결과 (실제 클래스/필드명)

- `Model/Order.h`: `struct Order { std::string id; std::string sampleId; std::string customerName; int quantity = 0; OrderStatus status = OrderStatus::RESERVED; std::string createdAt; };` — `OrderStatus`는 `RESERVED`/`PRODUCING`/`CONFIRMED`/`RELEASE`/`REJECTED` 5가지.
- `Repository/IOrderRepository.h`: `findById(id) -> optional<Order>`, `findAll() -> vector<Order>`, `save(order)`(upsert, 즉시 영속화). Phase 1의 `ISampleRepository`와 동일한 형태.
- `Service/ReleaseService`는 아직 미구현(Phase 4에서 Develope가 신규 작성) — 아래 §1에서 이 문서가 가정하는 시그니처를 명시한다.
- `MainController::handleRelease()`는 현재 TODO 스텁(`"[출고 처리] 기능은 Phase 4에서 구현됩니다."` 출력)만 존재(`Controller/MainController.cpp` 162~164행) — Phase 4에서 "출고 가능 주문 목록 조회 → 번호 선택 → 출고 처리" 메뉴로 채워야 한다.

## 1. `ReleaseService` 가정 시그니처 (Develope 구현 전 가정, §설계 결정 근거는 abnormal.md 1번 참고)

```cpp
class ReleaseService {
public:
    explicit ReleaseService(IOrderRepository& orderRepo);

    // CONFIRMED 상태 주문만 반환한다(출고 가능 목록).
    std::vector<Order> listReleasable() const;

    // orderId로 주문을 찾아 CONFIRMED 상태인 경우에만 RELEASE로 전이하고 저장한다.
    // - 주문이 없으면 std::invalid_argument("존재하지 않는 주문...")
    // - CONFIRMED가 아니면 std::invalid_argument("CONFIRMED 상태의 주문만 출고할 수 있습니다" 류)
    // - 성공 시 저장된(갱신된) Order를 반환한다.
    Order release(const std::string& orderId);

private:
    IOrderRepository& orderRepo_;
};
```

**`ISampleRepository`를 의도적으로 받지 않는다.** `docs/design.md` §4.1의 `IOrderState` 인터페이스 정의(84~90행)를 보면 `approve(Order&, SampleService&, ProductionService&)`는 재고/생산 서비스를 인자로 받지만, `release(Order& order)`는 **`Order` 하나만** 인자로 받는다 — 즉 설계 시점부터 출고 전이는 재고 계층에 접근할 방법 자체가 없도록 의도된 것으로 해석한다. 이 브랜치는 State 클래스가 아직 없으므로 `ReleaseService`가 직접 이 계약을 반영해, 애초에 `ISampleRepository` 의존성을 갖지 않는 것으로 가정했다. 자세한 근거와 대안(Mock으로 `Times(0)` 검증하는 방식과의 트레이드오프)은 `abnormal.md` 1번 항목 참고.

---

## A. 정상 출고 (`ReleaseService::release`)

### TC-P4-001: CONFIRMED 주문 정상 출고 (Happy Path)
- **대상 기능**: 출고 처리 — 상태 전이
- **Given**: `orderRepo`에 `Order{id:"ORD-001", sampleId:"S-001", customerName:"고객A", quantity:150, status:CONFIRMED}`가 시딩되어 있음.
- **When**: `ReleaseService::release("ORD-001")`를 호출한다.
- **Then**: 반환된 `Order.status == RELEASE`이고, 나머지 필드(`id`/`sampleId`/`customerName`/`quantity`/`createdAt`)는 변경되지 않는다. `orderRepo.save()`가 정확히 1회, `status == RELEASE`인 `Order`로 호출된다.
- **참조**: requirement.md 5.7절, design.md §4.1(205행)

### TC-P4-002: 출고 시 재고(Sample.stock)는 변화하지 않는다 (경계 확인)
- **대상 기능**: 출고 처리 — 재고 불변 경계
- **Given**: TC-P4-001과 동일한 CONFIRMED 주문.
- **When**: `ReleaseService::release("ORD-001")`를 호출한다.
- **Then**: `ReleaseService`는 `ISampleRepository`(또는 그에 준하는 재고 저장소)에 대한 어떤 참조도 생성자에서 받지 않으므로, 재고를 변경할 코드 경로 자체가 존재하지 않는다 — 즉 **컴파일 타임에 보장**된다. 이를 실제로 확인하려면 `ReleaseService`의 생성자 시그니처(`ISampleRepository` 파라미터 없음)를 Develope 구현체에서 확인하고, `Tests/ReleaseServiceTest.cpp`에서 `MockOrderRepository`만으로 전체 시나리오가 구성됨을 보임으로써 대체 검증한다(Mock `Times(0)` 방식이 아니라 "애초에 호출할 방법이 없음"을 보이는 더 강한 형태의 검증). Develope가 만약 `ISampleRepository`를 받는 구조로 구현한다면, 그 시점에 반드시 `save()`가 `Times(0)`으로 호출되지 않음을 회귀 테스트에 추가해야 한다(abnormal.md 1번 항목 참고).
- **참조**: requirement.md 5.6절(재고 반영은 승인/생산완료 시점에만), design.md §4.1

### TC-P4-003: 여러 CONFIRMED 주문 중 지정한 하나만 출고
- **대상 기능**: 출고 처리 — 다른 주문에 영향 없음
- **Given**: `orderRepo`에 CONFIRMED 상태 `ORD-001`, `ORD-002` 두 건이 있음.
- **When**: `ReleaseService::release("ORD-001")`만 호출한다.
- **Then**: `ORD-001`만 `RELEASE`로 전이되고 저장된다(`save`가 `ORD-001`에 대해서만 호출). `ORD-002`는 `findAll()`/`findById()` 스텁상 CONFIRMED 그대로이며 `save`가 호출되지 않는다.
- **참조**: requirement.md 5.7절

---

## B. 상태 가드 — CONFIRMED가 아닌 주문은 출고 거부

### TC-P4-010: RESERVED 상태 주문 출고 시도 거부
- **Given**: `Order{id:"ORD-010", status:RESERVED}`가 시딩되어 있음.
- **When**: `ReleaseService::release("ORD-010")`를 호출한다.
- **Then**: 예외(`std::invalid_argument`)가 발생하고, `orderRepo.save()`는 호출되지 않는다(`Times(0)`). 주문 상태는 `RESERVED`로 그대로 유지된다.
- **참조**: requirement.md 5.7절("CONFIRMED 상태 주문에 대해 출고 처리")

### TC-P4-011: PRODUCING 상태 주문 출고 시도 거부
- **Given**: `Order{id:"ORD-011", status:PRODUCING}`.
- **When/Then**: TC-P4-010과 동일하게 거부, `save` 미호출, 상태 유지.
- **참조**: requirement.md 5.7절

### TC-P4-012: REJECTED 상태 주문 출고 시도 거부
- **Given**: `Order{id:"ORD-012", status:REJECTED}`.
- **When/Then**: 동일하게 거부, `save` 미호출, 상태 유지.
- **참조**: requirement.md 5.7절, 5.5절(REJECTED는 정상 흐름 밖)

### TC-P4-013: 이미 RELEASE 상태인 주문 재출고(중복 출고) 거부
- **대상 기능**: 출고 처리 — 멱등성/중복 방지
- **Given**: `Order{id:"ORD-013", status:RELEASE}`(이미 출고 완료).
- **When**: `ReleaseService::release("ORD-013")`를 다시 호출한다.
- **Then**: 예외(`std::invalid_argument`) 발생, `save` 미호출. "CONFIRMED 상태의 주문만 출고할 수 있습니다" 류 메시지가 RESERVED/PRODUCING/REJECTED 거부와 동일한 경로로 처리된다(허용 상태가 CONFIRMED 하나뿐이므로 나머지 4개 상태 전부를 동일한 가드로 묶어 검증).
- **참조**: requirement.md 5.7절, design.md §4.1("`ConfirmedState`에서만 `ReleasedState`로 전이 가능")

---

## C. 존재하지 않는 주문번호

### TC-P4-020: 존재하지 않는 주문번호로 출고 시도
- **대상 기능**: 출고 처리 — 주문 조회 실패
- **Given**: `orderRepo.findById("ORD-999")`가 `std::nullopt`를 반환하도록 스텁됨(존재하지 않는 주문번호).
- **When**: `ReleaseService::release("ORD-999")`를 호출한다.
- **Then**: 예외(`std::invalid_argument`, "존재하지 않는 주문" 류 메시지)가 발생하고, `save`는 호출되지 않는다. 프로그램은 크래시 없이 에러를 표시하고 메뉴로 복귀해야 한다(Controller 레벨, TC-P4-042와 연계).
- **참조**: requirement.md 5.7절, design.md §8(공통 예외 처리 원칙)

---

## D. 출고 가능 목록 조회 (`ReleaseService::listReleasable`)

### TC-P4-030: CONFIRMED 상태 주문만 필터링되어 목록에 표시
- **대상 기능**: 출고 가능 주문 목록 조회
- **Given**: `orderRepo.findAll()`이 `RESERVED`, `PRODUCING`, `CONFIRMED`(2건), `RELEASE`, `REJECTED` 상태가 섞인 6건을 반환.
- **When**: `ReleaseService::listReleasable()`를 호출한다.
- **Then**: `CONFIRMED` 상태인 2건만 반환된다. 나머지 4건(RESERVED/PRODUCING/RELEASE/REJECTED)은 결과에서 제외된다.
- **참조**: requirement.md 5.7절("재고가 충분해진 CONFIRMED 주문에 대해 출고 처리"), docs/screens.md "[6] 출고 처리"("출고 가능 주문 (CONFIRMED)")

### TC-P4-031: CONFIRMED 주문이 없을 때 빈 목록
- **Given**: `orderRepo.findAll()`이 CONFIRMED가 아닌 주문들(또는 빈 벡터)만 반환.
- **When**: `ReleaseService::listReleasable()`를 호출한다.
- **Then**: 빈 벡터가 반환된다(크래시 없음). Controller 레벨에서는 "출고 가능한 주문이 없습니다" 류의 안내가 표시되어야 한다(TC-P4-041과 연계).
- **참조**: requirement.md 5.7절

---

## E. 콘솔 메뉴 통합 (`MainController::handleRelease`)

### TC-P4-040: 출고 가능 주문 목록에서 번호 선택 → 출고 완료 안내
- **대상 기능**: 메인 메뉴 [6] 출고 처리 — 전체 플로우
- **Given**: CONFIRMED 상태 주문 2건이 등록되어 있음(화면 예시 기준 `주문번호`/`고객`/`시료` 컬럼 존재).
- **When**: "6"을 입력해 출고 처리 메뉴로 진입 → 목록에서 "1"(첫 번째 CONFIRMED 주문)을 선택한다.
- **Then**: "출고 처리 완료" 안내와 함께 주문번호/출고수량(quantity)/처리일시/**상태(RELEASE)**가 표시된다. `docs/screens.md` 168~192행의 예시 화면 마지막 줄 "상태 CONFIRMED"는 오타로 간주하고(192행 주석 참고), 실제 구현은 반드시 `RELEASE`로 표시해야 한다 — 이 케이스로 그 회귀를 방지한다.
- **참조**: docs/screens.md "[6] 출고 처리", requirement.md 5.7절

### TC-P4-041: 출고 가능 주문이 없을 때 안내
- **대상 기능**: 출고 처리 메뉴 — 빈 목록 경계
- **Given**: CONFIRMED 상태 주문이 하나도 없음.
- **When**: 출고 처리 메뉴로 진입한다.
- **Then**: "출고 가능한 주문이 없습니다" 류의 안내 후 크래시 없이 메뉴로 복귀한다(번호 입력을 요구하지 않음).
- **참조**: requirement.md 5.7절

### TC-P4-042: 잘못된 번호 선택 시 에러 처리
- **대상 기능**: 출고 처리 메뉴 — 잘못된 입력/존재하지 않는 번호
- **Given**: CONFIRMED 상태 주문 2건(목록 번호 [1], [2])이 있음.
- **When**: 목록에 없는 번호(예: "9") 또는 숫자가 아닌 값("abc")을 입력한다.
- **Then**: 크래시 없이 "잘못된 입력입니다" 류의 메시지 표시 후 메뉴로 복귀(또는 재입력 요구)한다. `ReleaseService::release`가 호출되지 않거나, 호출되더라도 존재하지 않는 주문 처리(TC-P4-020)와 동일한 방식으로 처리되어야 한다.
- **참조**: design.md §8(EOF/잘못된 입력 공통 처리 원칙), docs_temp/Phase1/testcase.md TC-P1-031(동일 패턴)

---

## F. Repository 계층 유닛 테스트 커버리지 매핑

`Tests/ReleaseServiceTest.cpp`(gmock, `MockOrderRepository : public IOrderRepository`, 실제 파일 `SampleOrderSystem/Tests/Mocks/MockOrderRepository.h`)로 아래와 같이 작성했다.

| TestCase | 유닛 테스트 대상 | Mock 설정 포인트 |
|---|---|---|
| TC-P4-001, TC-P4-002 | `ReleaseService::release` 성공 경로(CONFIRMED → RELEASE), 재고 미접촉 | `MockOrderRepository::findById` → CONFIRMED `Order` 스텁, `save` 1회 호출 및 인자 캡처(`status == RELEASE`) 검증. `ISampleRepository` 자체를 생성자에 넘기지 않음으로써 재고 접촉 불가를 구조적으로 증명 |
| TC-P4-003 | 다건 중 지정 주문만 출고 | `findById`가 호출된 id에 따라 다른 `Order`를 반환하도록 스텁, 해당 id로만 `save` 호출됨을 검증 |
| TC-P4-010~013 | 상태 가드(비-CONFIRMED 4종 전부 거부) | `findById` → 각 상태의 `Order` 스텁, `EXPECT_THROW(..., std::invalid_argument)` + `EXPECT_CALL(repo, save(_)).Times(0)` |
| TC-P4-020 | 존재하지 않는 주문 | `findById` → `std::nullopt` 스텁, `EXPECT_THROW` + `save` `Times(0)` |
| TC-P4-030, TC-P4-031 | `ReleaseService::listReleasable` 필터링 | `findAll` → 혼합 상태/빈 벡터 스텁, 반환 값의 id 집합 검증 |

TC-P4-040~042(콘솔 메뉴 통합)는 유닛 테스트 범위 밖이며, Develope 구현 완료 후 `/system-test` 회귀 케이스로 이관하는 것을 권장한다(Phase 1의 TC-P1-001/010/020/032 이관 사례와 동일한 패턴).

## 다음 단계 (Develope 인계)

1. `ReleaseService::listReleasable`/`release`의 실제 시그니처를 이 문서의 §1 가정에 맞춰(또는 합리적 사유로 달리) 구현한다. `ISampleRepository`를 받지 않는 설계를 유지할지 여부는 abnormal.md 1번 항목을 참고해 결정한다 — 어느 쪽으로 결정하든 TC-P4-002의 "재고 미접촉" 계약만 지키면 된다.
2. `MainController::handleRelease()`를 TC-P4-040~042 기준 하위 플로우로 구현한다(현재 `Controller/MainController.cpp` 162~164행 TODO 스텁).
3. Debug 구성으로 `Tests/ReleaseServiceTest.cpp`를 vcxproj에 포함시켜 빌드/실행, 통과 여부 확인(Phase 1과 동일하게 `main.cpp`의 `_DEBUG` 시 `RUN_ALL_TESTS()` 경로에 자동 포함될 것으로 예상).
4. 구현 완료 후 tester가 실제 메뉴 문구를 확인해 `.claude/skills/system-test/run.ps1`의 `$cases`에 TC-P4-040/041 등을 추가하는 후속 회귀 작업을 진행한다(이번 커밋 범위 밖, Develope 완료 후 재요청 필요).
