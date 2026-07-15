// Phase 3 유닛 테스트 — IOrderState 전이 (docs_temp/Phase3/testcase.md TC-P3-001~043 대응)
//
// Domain/OrderState/*, Service/ProductionService, SampleService의 Phase 3 확장분은
// 아직 Develope가 구현하지 않았다. docs_temp/Phase3/testcase.md §0, abnormal.md
// 1~5번에 근거를 남긴 아래 가정 시그니처를 기준으로 작성했다.
//
//   class IOrderState {
//   public:
//       virtual ~IOrderState() = default;
//       virtual IOrderState& approve(Order& order, SampleService& sampleService,
//                                     ProductionService& productionService) const = 0;
//       virtual IOrderState& reject(Order& order) const = 0;
//       virtual IOrderState& completeProduction(Order& order, SampleService& sampleService) const = 0;
//       virtual IOrderState& release(Order& order) const = 0;
//       virtual OrderStatus statusCode() const = 0;
//   };
//   // ReservedState/ProducingState/ConfirmedState/ReleasedState/RejectedState 각각
//   // static IOrderState& instance(); 제공 (stateless singleton, design.md §10).
//   // 허용되지 않는 approve/reject/release 시도는 std::logic_error를 던진다
//   // (abnormal.md 1번 — 미확정, High 우선순위. no-op으로 확정되면 이 파일의
//   // TC-P3-020~022만 갱신하면 된다). completeProduction은 ProducingState 외에는
//   // no-op으로 *this를 반환한다(design.md §4.1에 명시적으로 서술, 모호함 없음).
//
//   class SampleService {
//       // ...(Phase 1 기존 registerSample/listAll/search)
//       std::optional<Sample> findById(const std::string& id) const;   // Phase 3 확장 가정
//       void adjustStock(const std::string& id, int delta);             // Phase 3 확장 가정
//   };
//
//   class ProductionService {
//   public:
//       ProductionService(IOrderRepository& orderRepo, IProductionQueueRepository& queueRepo,
//                          ISampleRepository& sampleRepo);              // abnormal.md 3번(3-인자 가정)
//       void enqueue(const ProductionQueueEntry& entry);
//       std::optional<ProductionQueueEntry> peekActive() const;
//       void completeActive();
//   };
//
// 실제 시그니처가 다르면 Develope 구현 후 tester가 이 파일과 testcase.md를 갱신한다.

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "../Domain/OrderState/IOrderState.h"
#include "../Domain/OrderState/ReservedState.h"
#include "../Domain/OrderState/ProducingState.h"
#include "../Domain/OrderState/ConfirmedState.h"
#include "../Domain/OrderState/ReleasedState.h"
#include "../Domain/OrderState/RejectedState.h"
#include "../Service/SampleService.h"
#include "../Service/ProductionService.h"
#include "Mocks/MockOrderRepository.h"
#include "Mocks/MockProductionQueueRepository.h"
#include "Mocks/MockSampleRepository.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

Sample MakeSample(const std::string& id, double avgProductionTime, double yield, int stock) {
    Sample s;
    s.id = id;
    s.name = "Wafer";
    s.avgProductionTime = avgProductionTime;
    s.yield = yield;
    s.stock = stock;
    return s;
}

Order MakeReservedOrder(const std::string& id, const std::string& sampleId, int quantity) {
    Order o;
    o.id = id;
    o.sampleId = sampleId;
    o.customerName = "Alice";
    o.quantity = quantity;
    o.status = OrderStatus::RESERVED;
    o.createdAt = "2026-07-15 09:00:00";
    return o;
}

}  // namespace

// ---- A. 재고로 전량 충당 (TC-P3-001, 002) ----

// TC-P3-001: 재고 200, 주문 150 -> 즉시 150 차감, CONFIRMED, 큐 미사용.
TEST(OrderStateTest, Approve_SufficientStock_DeductsImmediatelyAndConfirms) {
    NiceMock<MockSampleRepository> sampleRepo;
    NiceMock<MockOrderRepository> orderRepo;
    NiceMock<MockProductionQueueRepository> queueRepo;

    ON_CALL(sampleRepo, findById("S-001"))
        .WillByDefault(Return(MakeSample("S-001", 2.0, 0.9, 200)));

    Sample savedSample;
    EXPECT_CALL(sampleRepo, save(_)).WillOnce([&](const Sample& s) { savedSample = s; });
    EXPECT_CALL(queueRepo, save(_)).Times(0);

    SampleService sampleService(sampleRepo);
    ProductionService productionService(orderRepo, queueRepo, sampleRepo);

    Order order = MakeReservedOrder("ORD-1", "S-001", 150);
    IOrderState& next = ReservedState::instance().approve(order, sampleService, productionService);

    EXPECT_EQ(next.statusCode(), OrderStatus::CONFIRMED);
    EXPECT_EQ(order.status, OrderStatus::CONFIRMED);
    EXPECT_EQ(savedSample.stock, 50);
}

// TC-P3-002: 재고와 주문 수량이 정확히 일치 -> 재고 0, 큐 미사용.
TEST(OrderStateTest, Approve_ExactStockMatch_DeductsToZero) {
    NiceMock<MockSampleRepository> sampleRepo;
    NiceMock<MockOrderRepository> orderRepo;
    NiceMock<MockProductionQueueRepository> queueRepo;

    ON_CALL(sampleRepo, findById("S-001"))
        .WillByDefault(Return(MakeSample("S-001", 2.0, 0.9, 100)));

    Sample savedSample;
    EXPECT_CALL(sampleRepo, save(_)).WillOnce([&](const Sample& s) { savedSample = s; });
    EXPECT_CALL(queueRepo, save(_)).Times(0);

    SampleService sampleService(sampleRepo);
    ProductionService productionService(orderRepo, queueRepo, sampleRepo);

    Order order = MakeReservedOrder("ORD-1", "S-001", 100);
    IOrderState& next = ReservedState::instance().approve(order, sampleService, productionService);

    EXPECT_EQ(next.statusCode(), OrderStatus::CONFIRMED);
    EXPECT_EQ(savedSample.stock, 0);
}

// ---- B. 재고 부족(생산 필요) — 사례1/사례2 (TC-P3-003, 004, 005, 006) ----

// TC-P3-003 (requirement.md 5.6절 사례1 전반부): 재고 0, 부족분 50, 실생산량 ceil(50/0.5)=100.
TEST(OrderStateTest, Approve_Case1_ZeroStock_EnqueuesFullShortfallProduction) {
    NiceMock<MockSampleRepository> sampleRepo;
    NiceMock<MockOrderRepository> orderRepo;
    NiceMock<MockProductionQueueRepository> queueRepo;

    ON_CALL(sampleRepo, findById("S-001"))
        .WillByDefault(Return(MakeSample("S-001", /*avgProductionTime=*/1.0, /*yield=*/0.5, /*stock=*/0)));

    ProductionQueueEntry enqueued;
    EXPECT_CALL(queueRepo, save(_)).WillOnce([&](const ProductionQueueEntry& e) { enqueued = e; });

    SampleService sampleService(sampleRepo);
    ProductionService productionService(orderRepo, queueRepo, sampleRepo);

    Order order = MakeReservedOrder("ORD-1", "S-001", 50);
    IOrderState& next = ReservedState::instance().approve(order, sampleService, productionService);

    EXPECT_EQ(next.statusCode(), OrderStatus::PRODUCING);
    EXPECT_EQ(order.status, OrderStatus::PRODUCING);
    EXPECT_EQ(enqueued.orderId, "ORD-1");
    EXPECT_EQ(enqueued.sampleId, "S-001");
    EXPECT_EQ(enqueued.requiredQuantity, 100);            // ceil(50 / 0.5)
    EXPECT_DOUBLE_EQ(enqueued.totalProductionMinutes, 100.0); // avgProductionTime(1.0) * 100
}

// TC-P3-004 (사례1 후반부): completeActive로 재고 0->100 반영 후, 잔여 재고로 신규 주문 즉시 충당.
TEST(OrderStateTest, CompleteActiveThenApprove_Case1_LeftoverStockCoversNextOrder) {
    NiceMock<MockSampleRepository> sampleRepo;
    NiceMock<MockOrderRepository> orderRepo;
    NiceMock<MockProductionQueueRepository> queueRepo;

    ProductionQueueEntry entry;
    entry.orderId = "ORD-1";
    entry.sampleId = "S-001";
    entry.requiredQuantity = 100;
    entry.totalProductionMinutes = 100.0;
    entry.startedAt = "2000-01-01 00:00:00"; // 충분히 지난 과거 -> 완료 시점 도달

    ON_CALL(queueRepo, findAll()).WillByDefault(Return(std::vector<ProductionQueueEntry>{entry}));
    ON_CALL(orderRepo, findById("ORD-1"))
        .WillByDefault(Return(MakeReservedOrder("ORD-1", "S-001", 50)));
    Sample sampleState = MakeSample("S-001", 1.0, 0.5, 0);
    ON_CALL(sampleRepo, findById("S-001")).WillByDefault([&](const std::string&) {
        return std::optional<Sample>(sampleState);
    });
    ON_CALL(sampleRepo, save(_)).WillByDefault([&](const Sample& s) { sampleState = s; });

    EXPECT_CALL(queueRepo, remove("ORD-1")).Times(1);
    EXPECT_CALL(orderRepo, save(_)).Times(::testing::AtLeast(1));

    SampleService sampleService(sampleRepo);
    ProductionService productionService(orderRepo, queueRepo, sampleRepo);

    productionService.completeActive();

    EXPECT_EQ(sampleState.stock, 100); // 0 + 100, 수율 재적용 없음(규칙 4)

    // 잔여 재고로 새 주문(50개) 즉시 충당.
    ProductionQueueEntry* noNewEnqueue = nullptr;
    (void)noNewEnqueue;
    EXPECT_CALL(queueRepo, save(_)).Times(0);

    Order order2 = MakeReservedOrder("ORD-2", "S-001", 50);
    IOrderState& next = ReservedState::instance().approve(order2, sampleService, productionService);

    EXPECT_EQ(next.statusCode(), OrderStatus::CONFIRMED);
    EXPECT_EQ(sampleState.stock, 50); // 100 - 50
}

// TC-P3-005 (requirement.md 5.6절 사례2): 재고 50, 주문 100 -> 즉시 50 차감(재고 0), 부족분 50, 실생산량 100.
TEST(OrderStateTest, Approve_Case2_PartialStock_DeductsThenEnqueuesShortfall) {
    NiceMock<MockSampleRepository> sampleRepo;
    NiceMock<MockOrderRepository> orderRepo;
    NiceMock<MockProductionQueueRepository> queueRepo;

    ON_CALL(sampleRepo, findById("S-001"))
        .WillByDefault(Return(MakeSample("S-001", 1.0, 0.5, 50)));

    Sample savedSample;
    EXPECT_CALL(sampleRepo, save(_)).WillOnce([&](const Sample& s) { savedSample = s; });
    ProductionQueueEntry enqueued;
    EXPECT_CALL(queueRepo, save(_)).WillOnce([&](const ProductionQueueEntry& e) { enqueued = e; });

    SampleService sampleService(sampleRepo);
    ProductionService productionService(orderRepo, queueRepo, sampleRepo);

    Order order = MakeReservedOrder("ORD-2", "S-001", 100);
    IOrderState& next = ReservedState::instance().approve(order, sampleService, productionService);

    EXPECT_EQ(next.statusCode(), OrderStatus::PRODUCING);
    EXPECT_EQ(savedSample.stock, 0);           // 50 -> 0, 승인 즉시 전량 차감
    EXPECT_EQ(enqueued.requiredQuantity, 100); // ceil(50 / 0.5)
}

// TC-P3-006 (사례2 핵심): 주문2 생산 완료 이전에 접수된 주문3은 생산 큐를 무시하고
// 현재 재고(이미 0)만 기준으로 부족분을 재계산해 별도로 추가 생산한다.
TEST(OrderStateTest, Approve_Case2_IgnoresProductionQueue_DuplicatesShortfallProduction) {
    NiceMock<MockSampleRepository> sampleRepo;
    NiceMock<MockOrderRepository> orderRepo;
    NiceMock<MockProductionQueueRepository> queueRepo;

    // 주문2가 이미 재고를 0으로 소진한 뒤(사례2 전반부 완료 상태)의 재고.
    ON_CALL(sampleRepo, findById("S-001"))
        .WillByDefault(Return(MakeSample("S-001", 1.0, 0.5, 0)));

    // 큐에는 주문2용 생산 항목이 이미 대기/진행 중이라고 세팅해 두되,
    // 재고 확인 로직이 이를 절대 조회하지 않아야 함을 강조한다.
    ProductionQueueEntry order2Entry;
    order2Entry.orderId = "ORD-2";
    order2Entry.sampleId = "S-001";
    order2Entry.requiredQuantity = 100;
    order2Entry.totalProductionMinutes = 100.0;
    order2Entry.startedAt = "2026-07-15 10:00:00";
    ON_CALL(queueRepo, findAll()).WillByDefault(Return(std::vector<ProductionQueueEntry>{order2Entry}));

    EXPECT_CALL(sampleRepo, save(_)).Times(0); // 이미 재고 0이라 추가 차감 없음
    ProductionQueueEntry enqueued;
    EXPECT_CALL(queueRepo, save(_)).WillOnce([&](const ProductionQueueEntry& e) { enqueued = e; });

    SampleService sampleService(sampleRepo);
    ProductionService productionService(orderRepo, queueRepo, sampleRepo);

    Order order3 = MakeReservedOrder("ORD-3", "S-001", 50);
    IOrderState& next = ReservedState::instance().approve(order3, sampleService, productionService);

    EXPECT_EQ(next.statusCode(), OrderStatus::PRODUCING);
    EXPECT_EQ(enqueued.orderId, "ORD-3");
    EXPECT_EQ(enqueued.requiredQuantity, 100); // ceil(50 / 0.5), 주문2와 중복 생산(의도된 동작)
}

// ---- C. 거절 (TC-P3-010) ----

// TC-P3-010: RESERVED -> REJECTED, 재고/생산 큐 영향 없음.
TEST(OrderStateTest, Reject_ReservedOrder_TransitionsWithoutSideEffects) {
    NiceMock<MockSampleRepository> sampleRepo;
    EXPECT_CALL(sampleRepo, save(_)).Times(0);

    Order order = MakeReservedOrder("ORD-1", "S-001", 10);
    IOrderState& next = ReservedState::instance().reject(order);

    EXPECT_EQ(next.statusCode(), OrderStatus::REJECTED);
    EXPECT_EQ(order.status, OrderStatus::REJECTED);
}

// ---- D. 잘못된 상태 전이 (TC-P3-020~023) ----

// TC-P3-020 (가정: 예외): 이미 CONFIRMED인 주문에 승인 재시도.
TEST(OrderStateTest, Approve_AlreadyConfirmed_Throws) {
    NiceMock<MockSampleRepository> sampleRepo;
    NiceMock<MockOrderRepository> orderRepo;
    NiceMock<MockProductionQueueRepository> queueRepo;
    SampleService sampleService(sampleRepo);
    ProductionService productionService(orderRepo, queueRepo, sampleRepo);

    Order order = MakeReservedOrder("ORD-1", "S-001", 10);
    order.status = OrderStatus::CONFIRMED;

    EXPECT_THROW(ConfirmedState::instance().approve(order, sampleService, productionService), std::logic_error);
    EXPECT_EQ(order.status, OrderStatus::CONFIRMED);
}

// TC-P3-021 (가정: 예외): PRODUCING 상태에서 승인/거절 재시도.
TEST(OrderStateTest, ApproveAndReject_AlreadyProducing_Throws) {
    NiceMock<MockSampleRepository> sampleRepo;
    NiceMock<MockOrderRepository> orderRepo;
    NiceMock<MockProductionQueueRepository> queueRepo;
    SampleService sampleService(sampleRepo);
    ProductionService productionService(orderRepo, queueRepo, sampleRepo);

    Order order = MakeReservedOrder("ORD-1", "S-001", 10);
    order.status = OrderStatus::PRODUCING;

    EXPECT_THROW(ProducingState::instance().approve(order, sampleService, productionService), std::logic_error);
    EXPECT_THROW(ProducingState::instance().reject(order), std::logic_error);
    EXPECT_EQ(order.status, OrderStatus::PRODUCING);
}

// TC-P3-022 (가정: 예외): 이미 REJECTED인 주문에 거절 재시도.
TEST(OrderStateTest, Reject_AlreadyRejected_Throws) {
    Order order = MakeReservedOrder("ORD-1", "S-001", 10);
    order.status = OrderStatus::REJECTED;

    EXPECT_THROW(RejectedState::instance().reject(order), std::logic_error);
    EXPECT_EQ(order.status, OrderStatus::REJECTED);
}

// TC-P3-023 (확정: no-op): PRODUCING이 아닌 상태에서 completeProduction 호출은 아무 효과 없음.
TEST(OrderStateTest, CompleteProduction_NonProducingState_IsNoOp) {
    NiceMock<MockSampleRepository> sampleRepo;
    EXPECT_CALL(sampleRepo, save(_)).Times(0);
    SampleService sampleService(sampleRepo);

    Order order = MakeReservedOrder("ORD-1", "S-001", 10);
    order.status = OrderStatus::CONFIRMED;

    IOrderState& next = ConfirmedState::instance().completeProduction(order, sampleService);

    EXPECT_EQ(next.statusCode(), OrderStatus::CONFIRMED);
    EXPECT_EQ(order.status, OrderStatus::CONFIRMED);
}

// ---- F. 실 생산량 ceil 계산 경계값 (TC-P3-040~043) ----

// TC-P3-040: 수율 1.0 -> 실생산량 = 부족분 그대로.
TEST(OrderStateTest, Approve_YieldOne_RequiredQuantityEqualsShortfall) {
    NiceMock<MockSampleRepository> sampleRepo;
    NiceMock<MockOrderRepository> orderRepo;
    NiceMock<MockProductionQueueRepository> queueRepo;

    ON_CALL(sampleRepo, findById("S-001"))
        .WillByDefault(Return(MakeSample("S-001", 1.0, 1.0, 0)));

    ProductionQueueEntry enqueued;
    EXPECT_CALL(queueRepo, save(_)).WillOnce([&](const ProductionQueueEntry& e) { enqueued = e; });

    SampleService sampleService(sampleRepo);
    ProductionService productionService(orderRepo, queueRepo, sampleRepo);

    Order order = MakeReservedOrder("ORD-1", "S-001", 30);
    ReservedState::instance().approve(order, sampleService, productionService);

    EXPECT_EQ(enqueued.requiredQuantity, 30); // ceil(30 / 1.0)
}

// TC-P3-041: 수율이 매우 낮음(0.01) -> 큰 배율로 증폭.
TEST(OrderStateTest, Approve_VeryLowYield_AmplifiesRequiredQuantity) {
    NiceMock<MockSampleRepository> sampleRepo;
    NiceMock<MockOrderRepository> orderRepo;
    NiceMock<MockProductionQueueRepository> queueRepo;

    ON_CALL(sampleRepo, findById("S-001"))
        .WillByDefault(Return(MakeSample("S-001", 1.0, 0.01, 0)));

    ProductionQueueEntry enqueued;
    EXPECT_CALL(queueRepo, save(_)).WillOnce([&](const ProductionQueueEntry& e) { enqueued = e; });

    SampleService sampleService(sampleRepo);
    ProductionService productionService(orderRepo, queueRepo, sampleRepo);

    Order order = MakeReservedOrder("ORD-1", "S-001", 3);
    ReservedState::instance().approve(order, sampleService, productionService);

    EXPECT_EQ(enqueued.requiredQuantity, 300); // ceil(3 / 0.01)
}

// TC-P3-042: ceil 반올림 경계 (나누어떨어지지 않는 경우) — 화면 예시(54)가 아니라 정확한 ceil값(55) 검증.
TEST(OrderStateTest, Approve_CeilBoundary_RoundsUpNotScreenExampleValue) {
    NiceMock<MockSampleRepository> sampleRepo;
    NiceMock<MockOrderRepository> orderRepo;
    NiceMock<MockProductionQueueRepository> queueRepo;

    ON_CALL(sampleRepo, findById("S-001"))
        .WillByDefault(Return(MakeSample("S-001", 1.0, 0.92, 30)));

    Sample savedSample;
    EXPECT_CALL(sampleRepo, save(_)).WillOnce([&](const Sample& s) { savedSample = s; });
    ProductionQueueEntry enqueued;
    EXPECT_CALL(queueRepo, save(_)).WillOnce([&](const ProductionQueueEntry& e) { enqueued = e; });

    SampleService sampleService(sampleRepo);
    ProductionService productionService(orderRepo, queueRepo, sampleRepo);

    Order order = MakeReservedOrder("ORD-1", "S-001", 80);
    ReservedState::instance().approve(order, sampleService, productionService);

    EXPECT_EQ(savedSample.stock, 0);           // 30 -> 0
    EXPECT_EQ(enqueued.requiredQuantity, 55);  // ceil(50 / 0.92) = 55 (화면 예시의 54가 아님)
}

// TC-P3-043: 나누어떨어지는 경우 — 사례1과 동일 계산 재확인.
TEST(OrderStateTest, Approve_EvenlyDivisible_MatchesExactDivision) {
    NiceMock<MockSampleRepository> sampleRepo;
    NiceMock<MockOrderRepository> orderRepo;
    NiceMock<MockProductionQueueRepository> queueRepo;

    ON_CALL(sampleRepo, findById("S-001"))
        .WillByDefault(Return(MakeSample("S-001", 1.0, 0.5, 0)));

    ProductionQueueEntry enqueued;
    EXPECT_CALL(queueRepo, save(_)).WillOnce([&](const ProductionQueueEntry& e) { enqueued = e; });

    SampleService sampleService(sampleRepo);
    ProductionService productionService(orderRepo, queueRepo, sampleRepo);

    Order order = MakeReservedOrder("ORD-1", "S-001", 50);
    ReservedState::instance().approve(order, sampleService, productionService);

    EXPECT_EQ(enqueued.requiredQuantity, 100); // ceil(50 / 0.5)
}
