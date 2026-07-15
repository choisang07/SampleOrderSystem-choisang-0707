# Phase 2 테스트케이스 — 주문 접수 (시료 예약 → RESERVED)

범위: `docs/PLAN.md` Phase 2 = `OrderService::reserve` + `OrderFactory` + `ReservedState`(초기 상태) + 주문 접수 메뉴(`MainController` 하위, 화면 예시상 `[2] 시료 주문`).
관련 요구사항: `docs/requirement.md` 5.1절(메인 메뉴), 5.3절(시료 주문 — 예약 시 입력 값: 시료 ID/고객명/주문 수량), 4절(주문 상태 정의).
관련 설계: `docs/design.md` §3(패키지 구조 — `Domain/Factory/OrderFactory`, `Domain/OrderState/ReservedState`, `Service/OrderService`), §4.1(State 패턴), §4.4(Factory 패턴), §5(JSON 스키마·ID 포맷 `ORD-{YYYYMMDD}-{HHMMSS}`), §7(메뉴 구성), §9(Phase 매핑), §11.1(유닛 테스트 대상).
관련 화면: `docs/screens.md` "[2] 시료 주문" 예시.

**중요한 경계**: Phase 2는 순수하게 "예약(RESERVED) 상태로 주문을 생성"하는 것만 다룬다. 재고 확인/차감, 생산 큐 등록, 승인/거절 판단은 전부 Phase 3(`OrderService::approve`/`reject`, `ProductionService`)의 책임이며 Phase 2에서는 절대 발생하지 않는다. 이 문서의 모든 케이스는 `Sample.stock`이 얼마든(0이든 충분하든) 주문 접수 자체는 재고와 무관하게 항상 `RESERVED`로 생성되는 것을 전제로 한다.

## 0. Phase 0/1 이식 코드 확인 결과 (실제 클래스/필드명)

- `Model/Order.h`: `struct Order { std::string id; std::string sampleId; std::string customerName; int quantity = 0; OrderStatus status = OrderStatus::RESERVED; std::string createdAt; };` — `OrderStatus`는 `enum class`(`RESERVED`/`PRODUCING`/`CONFIRMED`/`RELEASE`/`REJECTED`)이다.
- **State 패턴 표현 방식 불일치 발견**: `docs/design.md` §4.1은 `Order`가 현재 상태를 `IOrderState*`(non-owning 참조/포인터)로 들고 `Order::status()`가 `state_->statusCode()`를 그대로 리턴한다고 서술하지만, 실제 Phase 0에서 이미 이식·확정된 `Model/Order.h`는 `IOrderState` 포인터 없이 순수 `enum class OrderStatus status` 필드만 가진다(Model = 데이터+JSON 매핑만, 로직 없음 원칙과 오히려 더 부합). 이 간극은 `docs_temp/Phase2/abnormal.md` 2번 항목에 기록했다 — Phase 2 자체 구현(주문 생성 시 `status = OrderStatus::RESERVED`로 초기화)에는 지장이 없으나, Phase 3에서 `IOrderState::approve()`가 `Order&`를 어떻게 갱신할지(직접 `order.status = ...` 대입 방식이 될 것으로 추정) 설계를 재확인해야 한다.
- `Repository/IOrderRepository.h`: `findById(id) -> optional<Order>`, `findAll() -> vector<Order>`, `save(order)`(upsert, 즉시 영속화). `JsonOrderRepository`는 Phase 0에서 이미 구현 완료.
- `Repository/ISampleRepository.h`: Phase 1과 동일(`findById`/`findAll`/`save`). `OrderFactory::create`가 "등록된 시료만 주문 가능"(design.md §4.4)을 검증할 때 `findById(sampleId)`를 사용한다.
- `Domain/Factory/OrderFactory`, `Domain/OrderState/*`(`IOrderState`, `ReservedState` 등), `Service/OrderService`는 **아직 미구현**(Phase 2에서 Develope가 신규 작성) — design.md §3, §4.1, §4.4, §5 기준으로 아래와 같이 시그니처를 가정해 테스트케이스/유닛테스트를 작성했다. 실제 시그니처가 다르면 회귀 단계에서 이 문서와 `Tests/*.cpp`를 갱신한다.
  ```cpp
  class OrderFactory {
  public:
      // 유효성 검증(시료 존재, 고객명 필수, 수량>0) 후 ID를 자동 생성(ORD-{YYYYMMDD}-{HHMMSS})해
      // RESERVED 상태의 Order를 생성한다. 검증 실패 시 std::invalid_argument를 던진다.
      static Order create(const std::string& sampleId, const std::string& customerName, int quantity,
                          const ISampleRepository& sampleRepo);
  };

  class OrderService {
  public:
      OrderService(IOrderRepository& orderRepo, ISampleRepository& sampleRepo);
      // OrderFactory로 검증/생성을 위임한 뒤 저장한다. 재고 확인/차감은 하지 않는다(Phase 3 책임).
      Order reserve(const std::string& sampleId, const std::string& customerName, int quantity);
  };
  ```
- **`MainController::handleOrder()` 현재 상태**: `"[주문 (접수/승인/거절)] 기능은 Phase 2~3에서 구현됩니다."`를 출력하는 TODO 스텁이다. **메뉴 번호/라우팅 자체가 이미 확정된 6개 메뉴 설계(design.md §7, docs/screens.md)와 어긋나 있음을 발견했다** — 자세한 내용과 Develope 조치 필요 사항은 `docs_temp/Phase2/abnormal.md` 1번 항목 참고. 콘솔 재현 케이스(TC-P2-030 이하)는 메뉴 번호가 이 불일치 해소 후 확정되므로, 화면 예시상의 "시료 ID > / 고객명 > / 주문 수량 >" 입력 흐름 자체만 검증 대상으로 삼고 메뉴 진입 번호는 Develope 구현 후 확정한다.

---

## A. 주문 접수 성공 경로 (`OrderFactory::create` + `OrderService::reserve`)

### TC-P2-001: 정상 예약 (Happy Path)
- **대상 기능**: 신규 주문 접수
- **Given**: 저장소에 시료 `S-001`(WaferA)이 등록되어 있음(재고는 0이든 임의의 값이든 무관 — Phase 2는 재고를 보지 않는다).
- **When**: 시료 ID="S-001", 고객명="삼성전자", 주문 수량=200을 입력해 예약을 실행한다.
- **Then**: `OrderService::reserve("S-001", "삼성전자", 200)`가 성공적으로 `Order{id:"ORD-{YYYYMMDD}-{HHMMSS}", sampleId:"S-001", customerName:"삼성전자", quantity:200, status:RESERVED}`를 생성해 `IOrderRepository::save`를 호출한다. 화면에는 생성된 주문번호와 현재 상태(`RESERVED`)가 표시된다(`docs/screens.md` "[2] 시료 주문" 예시와 동일한 흐름).
- **참조**: requirement.md 5.3절, design.md §4.4, §5(ID 포맷), screens.md "[2] 시료 주문"

### TC-P2-002: 재고 부족 상태에서도 접수는 성공 (Phase 2/3 경계 확인용 핵심 케이스)
- **대상 기능**: 예약 시점에 재고를 전혀 확인하지 않는다는 것을 명시적으로 검증
- **Given**: 시료 `S-001`의 `stock=0`(재고 없음).
- **When**: 시료 ID="S-001", 고객명="A대학연구실", 주문 수량=9999(재고를 압도적으로 초과하는 수량)로 예약을 실행한다.
- **Then**: 재고 부족과 무관하게 접수가 **그대로 성공**하고 `RESERVED` 상태의 `Order`가 생성된다. `Sample.stock`은 예약 과정에서 전혀 변경되지 않는다(`ISampleRepository::save`가 호출되지 않음 — 조회(`findById`)만 발생). "재고 확인/부족분 계산/생산 큐 등록"은 이 시점에 절대 일어나지 않는다(Phase 3 `OrderService::approve` 책임).
- **참조**: CLAUDE.md 3절("재고 확인 시 생산 큐는 절대 고려하지 않는다" — 이 케이스는 그 이전 단계인 "예약 시점에는 재고 확인 자체가 없다"는 것을 검증), requirement.md 5.3절/5.4절 경계, design.md §6(승인 처리는 `approve` 전용)

### TC-P2-003: 주문번호(ID) 포맷 검증
- **대상 기능**: `OrderFactory`의 주문 ID 자동 생성 — `ORD-{YYYYMMDD}-{HHMMSS}`
- **Given**: 임의의 등록된 시료 `S-001`.
- **When**: 예약을 실행한다(임의 시각에 실행).
- **Then**: 생성된 `Order.id`가 정규식 `^ORD-\d{8}-\d{6}$`에 매칭된다(날짜 8자리 + 시각 6자리, 예: `ORD-20260715-090000`). ID는 사용자가 입력하지 않고 시스템이 현재 wall-clock 시각 기준으로 채번한다(design.md §5 JSON 스키마 예시와 일치, Phase 1의 `S1`/`S-001` 포맷 오기 재발 방지를 위해 design.md §5/§8, screens.md 200행을 재대조함 — 세 문서 모두 `ORD-{YYYYMMDD}-{HHMMSS}`로 일관됨을 확인).
- **참조**: design.md §5, §8, screens.md "ID 포맷" 절
- **유의(테스트 구현 메모)**: ID가 실행 시각(wall-clock)에 의존하므로 유닛 테스트에서 정확한 문자열을 하드코딩하지 않고 정규식 매칭으로 검증한다(`Tests/OrderFactoryTest.cpp` 참고).

### TC-P2-004: `createdAt` 필드 기록 확인
- **대상 기능**: 주문 생성 시각 기록
- **Given**: 임의의 등록된 시료 `S-001`.
- **When**: 예약을 실행한다.
- **Then**: `Order.createdAt`이 `"YYYY-MM-DD HH:MM:SS"` 형식의 현재 시각으로 채워진다(빈 문자열이 아님). `Model/Order.h`의 기존 필드 주석과 일치.
- **참조**: design.md §3(Model/Order.h 필드), §5(JSON 스키마 `createdAt` 예시)

---

## B. 예외/경계 조건 — 시료 ID

### TC-P2-010: 존재하지 않는 시료 ID로 주문 시도
- **대상 기능**: `OrderFactory` 검증 — "등록된 시료만 주문 가능"(requirement.md 5.2절 원칙의 5.3절 연장)
- **Given**: 저장소에 `S-999`가 존재하지 않음.
- **When**: 시료 ID="S-999", 고객명="테스트", 수량=10으로 예약을 시도한다.
- **Then**: 예약이 거부되고 에러 메시지("등록되지 않은 시료입니다" 류)가 표시된다. `IOrderRepository::save`는 호출되지 않는다.
- **참조**: design.md §4.4(OrderFactory — "sampleId가 존재하지 않으면 예외"), requirement.md 5.2절

### TC-P2-011: 시료 ID 빈 문자열
- **대상 기능**: `OrderFactory` 검증 — 시료 ID 필수
- **Given**: 시료 ID 입력값이 빈 문자열("")인 경우.
- **When**: 예약을 시도한다.
- **Then**: 등록되지 않은 시료 취급으로 거부되거나("시료 ID를 입력하세요" 또는 "등록되지 않은 시료입니다" 중 하나) 에러 메시지가 표시된다. 저장소 변경 없음.
- **참조**: requirement.md 5.3절("예약 시 입력 값: 시료 ID")
- **비고**: 빈 시료 ID를 "필수 입력 누락"과 "존재하지 않는 시료"중 어느 메시지로 구분할지는 명시적 요구사항이 없어 `abnormal.md`에 기록했다(우선순위 Low — 사용자 경험 문구 차이일 뿐 동작 자체는 "거부"로 동일).

---

## C. 예외/경계 조건 — 고객명

### TC-P2-020: 고객명 빈 문자열 예약 거부
- **대상 기능**: `OrderFactory` 검증 — 고객명 필수
- **Given**: 고객명 입력값이 빈 문자열("") 또는 공백만("   ")인 경우, 시료 `S-001`은 정상 등록되어 있음.
- **When**: 시료 ID="S-001", 고객명="" (또는 "   "), 수량=10으로 예약을 시도한다.
- **Then**: 두 경우 모두 예약이 거부되고 "고객명을 입력하세요" 류의 에러 메시지가 표시된다. 저장소 변경 없음(`save` 호출 안 됨).
- **참조**: requirement.md 5.3절("예약 시 입력 값: 고객명"), Phase 1 `SampleFactory::isBlank`와 동일한 "필수 입력" 판정 기준 재사용을 가정(design.md는 재사용을 명시하지 않았으나 Phase 1 확정 스타일과의 일관성을 위해 제안 — `abnormal.md` 참고).

---

## D. 예외/경계 조건 — 주문 수량

### TC-P2-030: 주문 수량 0 예약 거부
- **대상 기능**: `OrderFactory` 검증 — 수량 유효 범위(0 초과)
- **Given**: 시료 `S-001` 정상 등록. 수량 입력값 = 0.
- **When**: 예약을 시도한다.
- **Then**: 예약이 거부되고 "주문 수량은 1 이상이어야 합니다" 류의 에러 메시지가 표시된다. 저장소 변경 없음.
- **참조**: requirement.md 5.3절, design.md §4.4(생성 시점 검증 원칙)

### TC-P2-031: 주문 수량 음수 예약 거부
- **대상 기능**: `OrderFactory` 검증 — 수량 유효 범위(음수 거부)
- **Given**: 시료 `S-001` 정상 등록. 수량 입력값 = -5.
- **When**: 예약을 시도한다.
- **Then**: 예약이 거부된다(TC-P2-030과 동일 사유). 저장소 변경 없음.
- **참조**: requirement.md 5.3절

### TC-P2-032: 주문 수량 경계값 1 (최소 양수) 예약 성공
- **대상 기능**: `OrderFactory` 검증 — 수량 하한 경계
- **Given**: 시료 `S-001` 정상 등록. 수량 입력값 = 1.
- **When**: 예약을 실행한다.
- **Then**: 예약이 성공하고 `Order.quantity == 1`인 `RESERVED` 주문이 생성된다.
- **참조**: requirement.md 5.3절

### TC-P2-033: 주문 수량에 숫자가 아닌 값 입력
- **대상 기능**: 입력 파싱(View/Controller 레벨) 예외 처리
- **Given**: 수량 입력값 = "abc"(숫자 파싱 불가).
- **When**: 예약을 시도한다.
- **Then**: 프로그램이 크래시하지 않고 "숫자를 입력하세요" 류의 에러 메시지를 표시한 뒤 메뉴로 복귀(또는 재입력 요구)한다. 저장소 변경 없음. (Phase 1 TC-P1-008과 동일한 처리 패턴을 재사용하는 것을 가정.)
- **참조**: design.md §8(EOF/예외 상황 공통 처리 원칙 연장선), requirement.md 5.3절

---

## E. 동일 시료에 대한 중복/다건 주문

### TC-P2-040: 동일 시료에 대해 서로 다른 고객이 여러 건 예약
- **대상 기능**: 주문 유일성 — 시료당 다건 주문 허용
- **Given**: 시료 `S-001`이 등록되어 있음.
- **When**: (1) 고객명="삼성전자", 수량=200으로 예약 → (2) 고객명="LG이노텍", 수량=50으로 예약(모두 같은 `S-001` 대상).
- **Then**: 서로 다른 주문번호(ID)를 가진 두 건의 `RESERVED` 주문이 각각 생성되어 저장소에 존재한다. 하나의 시료에 대해 동시에 여러 주문이 접수되는 것은 정상 흐름이다(요구사항에 "시료당 1건" 같은 제약이 없음).
- **참조**: requirement.md 5.3절, PRD.md 2장(도메인 모델 — Order는 독립 엔티티)

### TC-P2-041: 동일 초(second) 내 연속 예약 시 ID 충돌 가능성 (경계 케이스, abnormal 연계)
- **대상 기능**: 주문 ID 유일성 리스크 확인
- **Given**: 시료 `S-001`이 등록되어 있음.
- **When**: 매우 짧은 시간 간격(같은 초 내)으로 예약을 연속 2회 실행한다(테스트에서는 시각을 고정하는 방식으로 재현하거나, `OrderFactory`가 시각 이외의 채번 요소(순번 등)를 쓰는지 확인하는 형태로 작성).
- **Then**: 초 단위(`HHMMSS`)까지만 사용하는 현재 포맷(design.md §5)으로는 두 주문이 동일한 ID(`ORD-YYYYMMDD-HHMMSS`)를 갖게 될 수 있고, 이 경우 `IOrderRepository::save`(upsert 방식 — `JsonOrderRepository::save`는 동일 id를 찾으면 덮어씀)가 두 번째 주문을 첫 번째 주문 위에 덮어써 **첫 번째 주문이 유실**될 위험이 있다.
- **판정 보류**: 이 케이스는 실제로 재현 가능한지(초 단위 충돌 확률, 콘솔 조작 특성상 사람이 1초 내 두 번 입력을 완료하기 어려움)와 대응 방안(시퀀스 접미사 추가 등)이 확정되지 않아 `docs_temp/Phase2/abnormal.md` 3번 항목에 별도 기록했다. 유닛 테스트에서는 "동일 시각으로 두 번 생성 시 ID가 같다"는 사실 자체만 확인하고, 이를 결함으로 자동 실패 처리하지는 않는다(회귀 판정 보류, Develope/사용자 확인 필요).
- **참조**: design.md §5(ID 포맷), abnormal.md 3번 항목

---

## F. 콘솔 메뉴 통합 (`MainController` — 메뉴 번호는 abnormal.md 1번 해소 후 확정)

### TC-P2-050: 시료 주문 메뉴 입력 흐름 (확인 화면 포함)
- **대상 기능**: 화면 예시("[2] 시료 주문") 그대로의 입력 → 확인 → 확정 흐름
- **Given**: 프로그램이 정상 기동된 상태, 시료 `S-003`(파워 기판-6인치)이 등록되어 있음.
- **When**: 시료 주문 메뉴에서 시료 ID="S-003", 고객명="삼성전자", 수량=200을 입력한 뒤 확인 화면에서 `[Y]`(예약 접수)를 선택한다.
- **Then**: "예약 접수 완료." 메시지와 함께 생성된 주문번호, 현재 상태(`RESERVED`)가 표시된다(`docs/screens.md` "[2] 시료 주문" 예시와 동일한 문구/순서).
- **참조**: screens.md "[2] 시료 주문"
- **비고**: `[N]`(취소) 선택 시 동작(저장소 변경 없이 메뉴 복귀)은 화면 예시에 명시되어 있지 않아 Develope 구현 재량으로 남기고, 구현 확정 후 회귀 케이스로 추가한다.

### TC-P2-051: 확인 화면에서 취소(`[N]`) 선택
- **대상 기능**: 예약 확정 전 취소
- **Given**: 시료 주문 메뉴에서 시료 ID/고객명/수량 입력을 마치고 확인 화면이 표시된 상태.
- **When**: `[N]`(취소)을 선택한다.
- **Then**: 주문이 생성되지 않고(`IOrderRepository::save` 호출 없음) 메뉴로 복귀한다. 크래시 없음.
- **참조**: screens.md "[2] 시료 주문" 흐름(확인 단계 존재 자체는 명시, `[N]` 동작은 상식적 추정)

### TC-P2-052: 잘못된 메뉴 진입/복귀
- **대상 기능**: 시료 주문 메뉴 진입 및 뒤로가기
- **Given**: 메인 메뉴가 표시된 상태.
- **When**: 시료 주문 메뉴로 진입 후 입력 도중 EOF(표준입력 종료) 또는 취소가 발생한다.
- **Then**: 크래시 없이 상위 메뉴로 복귀한다(design.md §8 EOF 공통 처리 원칙, `MainController::handleSampleRegistration`과 동일 패턴 재사용 가정).
- **참조**: design.md §8

---

## G. Repository/Factory/Service 계층 유닛 테스트 커버리지 매핑

`Tests/OrderFactoryTest.cpp`, `Tests/OrderServiceTest.cpp`(gmock, `MockSampleRepository`/`MockOrderRepository`)로 아래와 같이 작성했다(실제 파일: `SampleOrderSystem/Tests/Mocks/MockOrderRepository.h`, `SampleOrderSystem/Tests/OrderFactoryTest.cpp`, `SampleOrderSystem/Tests/OrderServiceTest.cpp`).

| TestCase | 유닛 테스트 대상 | Mock 설정 포인트 |
|---|---|---|
| TC-P2-001, TC-P2-004 | `OrderFactory::create` 성공 경로(RESERVED 상태, createdAt 채워짐) | `MockSampleRepository::findById` → 존재하는 시료 스텁 |
| TC-P2-002 | `OrderService::reserve`가 재고를 조회/차감하지 않음 | `MockSampleRepository::save`가 **호출되지 않음**(`EXPECT_CALL(..., save(_)).Times(0)`)을 검증, `findById`만 `Times(1)` 이상 허용 |
| TC-P2-003 | `OrderFactory`의 ID 포맷(`ORD-\d{8}-\d{6}`) | 정규식(`std::regex`) 매칭, 실행 시각에 의존하므로 하드코딩 값 비교 금지 |
| TC-P2-010, TC-P2-011 | `OrderFactory` 시료 존재 검증 | `MockSampleRepository::findById` → `std::nullopt` 스텁(TC-P2-010), 빈 ID 그대로 조회(TC-P2-011) |
| TC-P2-020 | `OrderFactory` 고객명 필수 검증 | `MockSampleRepository::findById` → 존재하는 시료 스텁(고객명 검증이 시료 검증보다 먼저/나중 어느 쪽이든 결과는 거부이므로 순서 무관하게 검증) |
| TC-P2-030~032 | `OrderFactory` 수량 유효 범위(>0) 검증 | 순수 값 검증(경계값 0/-5/1) |
| TC-P2-040 | `OrderService::reserve` 다건 생성 시 서로 다른 주문번호 | 동일 `sampleId`로 `reserve` 2회 호출, `save`가 서로 다른 `id` 인자로 `Times(2)` 호출됨을 확인(캡처 방식) |
| TC-P2-041 | ID 시각 충돌 가능성 확인(회귀 실패 처리 안 함, 정보성 테스트) | 동일 초 내 2회 호출 시뮬레이션이 어려우므로(실제 wall-clock 의존) 유닛 테스트에서는 스킵하거나 주석으로 리스크만 남김 — 아래 F 절 비고 참고 |

시스템 테스트(`/system-test`) 이관 후보는 TC-P2-001(정상 예약, 화면 확인 포함), TC-P2-050(확인 화면 포함 전체 흐름)이다. 다만 위 F절 비고대로 실제 메뉴 문구/입력 순서가 아직 Develope 구현 전이므로, `run.ps1` `$cases` 반영은 Phase 1과 동일한 기준으로 **보류**한다(메뉴 번호 확정 및 실제 문구 채취 후 추가).

---

## 다음 단계 (Develope 인계)

1. `docs_temp/Phase2/abnormal.md` 1번 항목(메뉴 번호 불일치)을 먼저 확인하고, `MainController`의 `run()` 분기를 design.md §7/screens.md 6개 메뉴 설계와 일치시킬지 여부를 결정한 뒤 Phase 2 메뉴(`[2] 시료 주문`)를 그 결정에 맞춰 구현한다.
2. `OrderFactory::create`, `OrderService::reserve`의 실제 시그니처를 구현하고, 이 문서의 가정과 다르면 회귀 단계에서 본 문서와 `Tests/*.cpp`를 함께 갱신한다.
3. `Domain/OrderState/IOrderState.h`, `ReservedState.h/.cpp`를 design.md §4.1 기준으로 최소 골격(다른 상태 전이 메서드는 Phase 3에서 구현하되, "현재 상태 조회"만 Phase 2에서 필요하다면)으로 추가할지, 아니면 Phase 2는 `Model/Order.h`의 `OrderStatus` enum만으로 충분히 처리하고 `IOrderState` 계층 자체는 Phase 3에서 도입할지 판단한다(abnormal.md 2번 항목의 Model 표현 방식 불일치와 연계된 결정 사항).
4. Debug 구성으로 `Tests/OrderFactoryTest.cpp`, `Tests/OrderServiceTest.cpp`를 빌드/실행해 통과 여부 확인(vcxproj에 신규 파일 포함).
5. 구현 완료 후 tester(TestCodeDeveloper)가 실제 메뉴 문구를 확인해 `run.ps1`의 `$cases`에 TC-P2-001/TC-P2-050 등을 추가한다.
