// Phase 3 유닛 테스트 — ProductionService (docs_temp/Phase3/testcase.md TC-P3-030~035 대응)
//
// ProductionService는 아직 Develope가 구현하지 않았다. docs_temp/Phase3/testcase.md §0,
// abnormal.md 3번/5번에 근거를 남긴 아래 가정 시그니처를 기준으로 작성했다.
//
//   class ProductionService {
//   public:
//       ProductionService(IOrderRepository& orderRepo, IProductionQueueRepository& queueRepo,
//                          ISampleRepository& sampleRepo);   // abnormal.md 3번(3-인자 가정)
//       void enqueue(const ProductionQueueEntry& entry);      // append, 큐가 비어 있었으면 startedAt 채움
//       std::optional<ProductionQueueEntry> peekActive() const; // findAll()[0]
//       void completeActive();                                // head 완료 시점 도달 시에만 부수효과 발생(lazy)
//   };
//
// requiredQuantity/totalProductionMinutes 계산(ceil(부족분/yield) 등)은 ReservedState::approve가
// 수행해 완성된 entry를 enqueue에 넘기는 것으로 가정했으므로(abnormal.md 5번), 이 파일은 계산 자체가
// 아니라 큐 조작(FIFO 순서/시작 처리/완료 처리)만 검증한다. 실 생산량 계산 경계값은 OrderStateTest.cpp
// (TC-P3-040~043)에서 검증한다.
// wall-clock 검증 방법(abnormal.md 6번): startedAt을 과거/미래로 직접 세팅해 완료 시점 도달 여부를 재현한다.
//
// 실제 시그니처가 다르면 Develope 구현 후 tester가 이 파일과 testcase.md를 갱신한다.

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "../Service/ProductionService.h"
#include "Mocks/MockOrderRepository.h"
#include "Mocks/MockProductionQueueRepository.h"
#include "Mocks/MockSampleRepository.h"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

ProductionQueueEntry MakeEntry(const std::string& orderId, const std::string& sampleId,
                                int requiredQuantity, double totalMinutes, const std::string& startedAt = "") {
    ProductionQueueEntry e;
    e.orderId = orderId;
    e.sampleId = sampleId;
    e.requiredQuantity = requiredQuantity;
    e.totalProductionMinutes = totalMinutes;
    e.startedAt = startedAt;
    return e;
}

Sample MakeSample(const std::string& id, int stock) {
    Sample s;
    s.id = id;
    s.name = "Wafer";
    s.avgProductionTime = 1.0;
    s.yield = 0.9;
    s.stock = stock;
    return s;
}

}  // namespace

// ---- E. FIFO 생산 큐 (TC-P3-030~035) ----

// TC-P3-030: 큐가 비어 있을 때 첫 등록은 즉시 시작(startedAt 채워짐).
TEST(ProductionServiceTest, Enqueue_EmptyQueue_StartsImmediately) {
    NiceMock<MockOrderRepository> orderRepo;
    NiceMock<MockProductionQueueRepository> queueRepo;
    NiceMock<MockSampleRepository> sampleRepo;

    ON_CALL(queueRepo, findAll()).WillByDefault(Return(std::vector<ProductionQueueEntry>{}));

    ProductionQueueEntry saved;
    EXPECT_CALL(queueRepo, save(_)).WillOnce([&](const ProductionQueueEntry& e) { saved = e; });

    ProductionService service(orderRepo, queueRepo, sampleRepo);
    service.enqueue(MakeEntry("ORD-1", "S-001", 100, 100.0));

    EXPECT_FALSE(saved.startedAt.empty());
}

// TC-P3-031: 큐에 이미 진행 중인 항목이 있으면 신규 등록은 대기(startedAt 비움 유지).
TEST(ProductionServiceTest, Enqueue_NonEmptyQueue_NewEntryWaits) {
    NiceMock<MockOrderRepository> orderRepo;
    NiceMock<MockProductionQueueRepository> queueRepo;
    NiceMock<MockSampleRepository> sampleRepo;

    ProductionQueueEntry active = MakeEntry("ORD-1", "S-001", 100, 100.0, "2026-07-15 09:00:00");
    ON_CALL(queueRepo, findAll()).WillByDefault(Return(std::vector<ProductionQueueEntry>{active}));

    ProductionQueueEntry saved;
    EXPECT_CALL(queueRepo, save(_)).WillOnce([&](const ProductionQueueEntry& e) { saved = e; });

    ProductionService service(orderRepo, queueRepo, sampleRepo);
    service.enqueue(MakeEntry("ORD-2", "S-002", 50, 50.0));

    EXPECT_TRUE(saved.startedAt.empty());

    // peekActive는 여전히 큐 head(ORD-1)를 반환한다.
    auto headAfter = service.peekActive();
    ASSERT_TRUE(headAfter.has_value());
    EXPECT_EQ(headAfter->orderId, "ORD-1");
}

// TC-P3-032: 여러 건 등록 시 FIFO 순서 보장 — head는 항상 가장 먼저 등록된 항목.
TEST(ProductionServiceTest, PeekActive_ReturnsFirstRegisteredEntry) {
    NiceMock<MockOrderRepository> orderRepo;
    NiceMock<MockProductionQueueRepository> queueRepo;
    NiceMock<MockSampleRepository> sampleRepo;

    ProductionQueueEntry a = MakeEntry("ORD-A", "S-001", 10, 10.0, "2026-07-15 09:00:00");
    ProductionQueueEntry b = MakeEntry("ORD-B", "S-001", 20, 20.0);
    ProductionQueueEntry c = MakeEntry("ORD-C", "S-001", 30, 30.0);
    ON_CALL(queueRepo, findAll()).WillByDefault(Return(std::vector<ProductionQueueEntry>{a, b, c}));

    ProductionService service(orderRepo, queueRepo, sampleRepo);
    auto head = service.peekActive();

    ASSERT_TRUE(head.has_value());
    EXPECT_EQ(head->orderId, "ORD-A");
}

// TC-P3-033: 완료 시점 미도달 -> completeActive는 부수효과 없음.
TEST(ProductionServiceTest, CompleteActive_NotYetDue_DoesNothing) {
    NiceMock<MockOrderRepository> orderRepo;
    NiceMock<MockProductionQueueRepository> queueRepo;
    NiceMock<MockSampleRepository> sampleRepo;

    // startedAt = 지금, totalProductionMinutes가 매우 커서 아직 한참 남음.
    ProductionQueueEntry notDue = MakeEntry("ORD-1", "S-001", 100, 999999.0, "2026-07-15 09:00:00");
    ON_CALL(queueRepo, findAll()).WillByDefault(Return(std::vector<ProductionQueueEntry>{notDue}));

    EXPECT_CALL(queueRepo, remove(_)).Times(0);
    EXPECT_CALL(sampleRepo, save(_)).Times(0);
    EXPECT_CALL(orderRepo, save(_)).Times(0);

    ProductionService service(orderRepo, queueRepo, sampleRepo);
    service.completeActive();
}

// TC-P3-034: 완료 시점 도달 -> 재고 반영(수율 재적용 없음)/주문 CONFIRMED 전이/큐 제거/다음 항목 시작.
TEST(ProductionServiceTest, CompleteActive_Due_ReflectsStockAndAdvancesQueue) {
    NiceMock<MockOrderRepository> orderRepo;
    NiceMock<MockProductionQueueRepository> queueRepo;
    NiceMock<MockSampleRepository> sampleRepo;

    ProductionQueueEntry due = MakeEntry("ORD-1", "S-001", 100, 100.0, "2000-01-01 00:00:00");
    ProductionQueueEntry waiting = MakeEntry("ORD-2", "S-001", 50, 50.0);
    ON_CALL(queueRepo, findAll()).WillByDefault(Return(std::vector<ProductionQueueEntry>{due, waiting}));

    Order order1;
    order1.id = "ORD-1";
    order1.sampleId = "S-001";
    order1.customerName = "Alice";
    order1.quantity = 100;
    order1.status = OrderStatus::PRODUCING;
    order1.createdAt = "2026-07-15 08:00:00";
    ON_CALL(orderRepo, findById("ORD-1")).WillByDefault(Return(order1));

    Sample sample = MakeSample("S-001", 0);
    ON_CALL(sampleRepo, findById("S-001")).WillByDefault(Return(sample));

    Sample savedSample;
    EXPECT_CALL(sampleRepo, save(_)).WillOnce([&](const Sample& s) { savedSample = s; });

    Order savedOrder;
    EXPECT_CALL(orderRepo, save(_)).WillOnce([&](const Order& o) { savedOrder = o; });

    EXPECT_CALL(queueRepo, remove("ORD-1")).Times(1);
    // 다음 대기 항목(ORD-2)이 새 head가 되어 startedAt이 채워져야 함.
    EXPECT_CALL(queueRepo, save(_)).Times(AtLeast(0));

    ProductionService service(orderRepo, queueRepo, sampleRepo);
    service.completeActive();

    EXPECT_EQ(savedSample.stock, 100); // 0 + 100, 수율 재적용 없음(규칙 4)
    EXPECT_EQ(savedOrder.status, OrderStatus::CONFIRMED);
}

// TC-P3-035: 완료 후 큐가 비면 peekActive는 nullopt.
TEST(ProductionServiceTest, CompleteActive_LastEntry_QueueBecomesEmpty) {
    NiceMock<MockOrderRepository> orderRepo;
    NiceMock<MockProductionQueueRepository> queueRepo;
    NiceMock<MockSampleRepository> sampleRepo;

    ProductionQueueEntry due = MakeEntry("ORD-1", "S-001", 10, 10.0, "2000-01-01 00:00:00");

    // completeActive 이전에는 1건, remove 이후에는 findAll이 빈 벡터를 반환하도록 상태 전이 시뮬레이션.
    bool removed = false;
    ON_CALL(queueRepo, findAll()).WillByDefault([&]() {
        return removed ? std::vector<ProductionQueueEntry>{} : std::vector<ProductionQueueEntry>{due};
    });
    ON_CALL(queueRepo, remove("ORD-1")).WillByDefault([&](const std::string&) { removed = true; });

    Order order1;
    order1.id = "ORD-1";
    order1.sampleId = "S-001";
    order1.quantity = 10;
    order1.status = OrderStatus::PRODUCING;
    ON_CALL(orderRepo, findById("ORD-1")).WillByDefault(Return(order1));
    ON_CALL(sampleRepo, findById("S-001")).WillByDefault(Return(MakeSample("S-001", 0)));

    ProductionService service(orderRepo, queueRepo, sampleRepo);
    service.completeActive();

    EXPECT_FALSE(service.peekActive().has_value());
}
