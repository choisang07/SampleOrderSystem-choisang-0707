// Phase 5 유닛 테스트 — MonitorService (docs_temp/Phase5/testcase.md TC-P5-001~032 대응)
//
// MonitorService는 아직 Develope가 구현하지 않았다(design.md §3, §6-5 기준으로 아래 시그니처를 가정).
//   enum class InventoryLevel { SUFFICIENT, SHORTAGE, DEPLETED }; // 여유 / 부족 / 고갈
//   struct SampleInventoryStatus {
//       std::string sampleId;
//       std::string sampleName;
//       int stock;
//       int pendingDemand;      // 해당 시료에 대한 RESERVED 주문 수량 합 (design.md §8 확정)
//       InventoryLevel level;
//   };
//   struct MonitoringSummary {
//       int sampleCount;
//       int totalStock;
//       int totalOrderCount;
//       int productionQueueCount;
//   };
//   class MonitorService {
//   public:
//       MonitorService(IOrderRepository& orderRepo, ISampleRepository& sampleRepo,
//                      IProductionQueueRepository& queueRepo);
//       std::vector<Order> ordersByStatus(OrderStatus status) const;
//       std::map<OrderStatus, int> countAllStatuses() const;   // REJECTED 키 없음
//       std::vector<SampleInventoryStatus> inventoryStatus() const;
//       MonitoringSummary summary() const;
//   };
// 병렬 개발 전략(CLAUDE.md, 이번 작업 지시): Phase 2~4 Service/State 클래스는 이 브랜치에 없으므로,
// Order/Sample/ProductionQueueEntry를 Mock Repository에 직접 시딩해 상태 조합을 재현한다.
// 실제 시그니처가 다르면 Develope 구현 후 tester가 이 파일을 갱신한다.

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "../Service/MonitorService.h"
#include "Mocks/MockOrderRepository.h"
#include "Mocks/MockProductionQueueRepository.h"
#include "Mocks/MockSampleRepository.h"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

Order MakeOrder(const std::string& id, const std::string& sampleId, OrderStatus status, int quantity) {
    Order o;
    o.id = id;
    o.sampleId = sampleId;
    o.customerName = "고객";
    o.quantity = quantity;
    o.status = status;
    o.createdAt = "2026-07-15 09:00:00";
    return o;
}

Sample MakeSample(const std::string& id, const std::string& name, int stock) {
    Sample s;
    s.id = id;
    s.name = name;
    s.avgProductionTime = 1.0;
    s.yield = 0.9;
    s.stock = stock;
    return s;
}

ProductionQueueEntry MakeQueueEntry(const std::string& orderId, const std::string& sampleId,
                                     const std::string& startedAt = "") {
    ProductionQueueEntry e;
    e.orderId = orderId;
    e.sampleId = sampleId;
    e.requiredQuantity = 1;
    e.totalProductionMinutes = 1.0;
    e.startedAt = startedAt;
    return e;
}

// 공통 픽스처: 3개 Mock Repository를 NiceMock으로 준비하고, findAll() 기본값을 빈 벡터로 설정한다.
class MonitorServiceTest : public ::testing::Test {
protected:
    NiceMock<MockOrderRepository> orderRepo;
    NiceMock<MockSampleRepository> sampleRepo;
    NiceMock<MockProductionQueueRepository> queueRepo;

    void SetUp() override {
        ON_CALL(orderRepo, findAll()).WillByDefault(Return(std::vector<Order>{}));
        ON_CALL(sampleRepo, findAll()).WillByDefault(Return(std::vector<Sample>{}));
        ON_CALL(queueRepo, findAll()).WillByDefault(Return(std::vector<ProductionQueueEntry>{}));
    }

    void SeedOrders(std::vector<Order> orders) {
        ON_CALL(orderRepo, findAll()).WillByDefault(Return(std::move(orders)));
    }

    void SeedSamples(std::vector<Sample> samples) {
        ON_CALL(sampleRepo, findAll()).WillByDefault(Return(std::move(samples)));
    }

    void SeedQueue(std::vector<ProductionQueueEntry> entries) {
        ON_CALL(queueRepo, findAll()).WillByDefault(Return(std::move(entries)));
    }
};

std::vector<std::string> IdsOf(const std::vector<Order>& orders) {
    std::vector<std::string> ids;
    ids.reserve(orders.size());
    for (const auto& o : orders) ids.push_back(o.id);
    return ids;
}

}  // namespace

// ---- A. 상태별 주문 현황 (TC-P5-001~005) ----

// TC-P5-001: REJECTED를 제외한 4개 상태만 집계한다.
TEST_F(MonitorServiceTest, CountAllStatuses_ExcludesRejected) {
    SeedOrders({
        MakeOrder("ORD-001", "S-001", OrderStatus::RESERVED, 1),
        MakeOrder("ORD-002", "S-001", OrderStatus::RESERVED, 1),
        MakeOrder("ORD-003", "S-001", OrderStatus::CONFIRMED, 1),
        MakeOrder("ORD-004", "S-001", OrderStatus::PRODUCING, 1),
        MakeOrder("ORD-005", "S-001", OrderStatus::PRODUCING, 1),
        MakeOrder("ORD-006", "S-001", OrderStatus::PRODUCING, 1),
        MakeOrder("ORD-007", "S-001", OrderStatus::RELEASE, 1),
        MakeOrder("ORD-008", "S-001", OrderStatus::REJECTED, 1),
        MakeOrder("ORD-009", "S-001", OrderStatus::REJECTED, 1),
    });

    MonitorService service(orderRepo, sampleRepo, queueRepo);
    const auto counts = service.countAllStatuses();

    EXPECT_EQ(counts.count(OrderStatus::REJECTED), 0u) << "REJECTED는 키 자체가 없어야 한다";
    ASSERT_TRUE(counts.count(OrderStatus::RESERVED));
    ASSERT_TRUE(counts.count(OrderStatus::CONFIRMED));
    ASSERT_TRUE(counts.count(OrderStatus::PRODUCING));
    ASSERT_TRUE(counts.count(OrderStatus::RELEASE));
    EXPECT_EQ(counts.at(OrderStatus::RESERVED), 2);
    EXPECT_EQ(counts.at(OrderStatus::CONFIRMED), 1);
    EXPECT_EQ(counts.at(OrderStatus::PRODUCING), 3);
    EXPECT_EQ(counts.at(OrderStatus::RELEASE), 1);
}

// TC-P5-002 (잠정 채택, abnormal.md 4번): REJECTED로 명시 조회하면 빈 목록.
TEST_F(MonitorServiceTest, OrdersByStatus_Rejected_ReturnsEmpty) {
    SeedOrders({
        MakeOrder("ORD-001", "S-001", OrderStatus::REJECTED, 1),
        MakeOrder("ORD-002", "S-001", OrderStatus::REJECTED, 1),
    });

    MonitorService service(orderRepo, sampleRepo, queueRepo);

    EXPECT_TRUE(service.ordersByStatus(OrderStatus::REJECTED).empty());
}

// TC-P5-003: 특정 상태로 조회 시 해당 상태 주문만 정확히 반환.
TEST_F(MonitorServiceTest, OrdersByStatus_ReturnsOnlyMatchingStatus) {
    SeedOrders({
        MakeOrder("ORD-001", "S-001", OrderStatus::RESERVED, 1),
        MakeOrder("ORD-002", "S-001", OrderStatus::RESERVED, 1),
        MakeOrder("ORD-003", "S-001", OrderStatus::PRODUCING, 1),
    });

    MonitorService service(orderRepo, sampleRepo, queueRepo);
    const auto reserved = service.ordersByStatus(OrderStatus::RESERVED);

    EXPECT_THAT(IdsOf(reserved), ::testing::UnorderedElementsAre("ORD-001", "ORD-002"));
}

// TC-P5-004: 특정 상태 주문이 하나도 없으면 0건/빈 목록 (키 유무는 관대하게 허용).
TEST_F(MonitorServiceTest, NoOrdersOfStatus_ReturnsZeroAndEmpty) {
    SeedOrders({
        MakeOrder("ORD-001", "S-001", OrderStatus::RESERVED, 1),
        MakeOrder("ORD-002", "S-001", OrderStatus::CONFIRMED, 1),
    });

    MonitorService service(orderRepo, sampleRepo, queueRepo);
    const auto counts = service.countAllStatuses();
    const int producingCount = counts.count(OrderStatus::PRODUCING) ? counts.at(OrderStatus::PRODUCING) : 0;

    EXPECT_EQ(producingCount, 0);
    EXPECT_TRUE(service.ordersByStatus(OrderStatus::PRODUCING).empty());
}

// TC-P5-005: PRODUCING 카운트와 생산큐 크기는 서로 다른 저장소에서 독립적으로 집계된다(교차검증 없음).
TEST_F(MonitorServiceTest, ProducingCount_And_QueueCount_AreIndependent) {
    SeedOrders({
        MakeOrder("ORD-001", "S-001", OrderStatus::PRODUCING, 1),
        MakeOrder("ORD-002", "S-001", OrderStatus::PRODUCING, 1),
        MakeOrder("ORD-003", "S-001", OrderStatus::PRODUCING, 1),
    });
    SeedQueue({
        MakeQueueEntry("ORD-001", "S-001", "2026-07-15 09:00:00"),
        MakeQueueEntry("ORD-002", "S-001"),
    });

    MonitorService service(orderRepo, sampleRepo, queueRepo);

    EXPECT_EQ(service.countAllStatuses().at(OrderStatus::PRODUCING), 3);
    EXPECT_EQ(service.summary().productionQueueCount, 2);
}

// ---- B. 재고 현황 / 대기 수요 (TC-P5-010~016) ----

// TC-P5-010: 재고가 대기 수요보다 많으면 여유.
TEST_F(MonitorServiceTest, InventoryStatus_SufficientWhenStockExceedsDemand) {
    SeedSamples({MakeSample("S-001", "WaferA", 10)});
    SeedOrders({
        MakeOrder("ORD-001", "S-001", OrderStatus::RESERVED, 3),
        MakeOrder("ORD-002", "S-001", OrderStatus::RESERVED, 2),
    });

    MonitorService service(orderRepo, sampleRepo, queueRepo);
    const auto status = service.inventoryStatus();

    ASSERT_EQ(status.size(), 1u);
    EXPECT_EQ(status[0].sampleId, "S-001");
    EXPECT_EQ(status[0].stock, 10);
    EXPECT_EQ(status[0].pendingDemand, 5);
    EXPECT_EQ(status[0].level, InventoryLevel::SUFFICIENT);
}

// TC-P5-011: 재고가 대기 수요보다 적으면(0은 아님) 부족.
TEST_F(MonitorServiceTest, InventoryStatus_ShortageWhenStockBelowDemand) {
    SeedSamples({MakeSample("S-002", "WaferB", 3)});
    SeedOrders({MakeOrder("ORD-001", "S-002", OrderStatus::RESERVED, 10)});

    MonitorService service(orderRepo, sampleRepo, queueRepo);
    const auto status = service.inventoryStatus();

    ASSERT_EQ(status.size(), 1u);
    EXPECT_EQ(status[0].pendingDemand, 10);
    EXPECT_EQ(status[0].level, InventoryLevel::SHORTAGE);
}

// TC-P5-012: 재고가 정확히 0이면 대기 수요와 무관하게 고갈(우선 적용).
TEST_F(MonitorServiceTest, InventoryStatus_DepletedWhenStockIsZero_RegardlessOfDemand) {
    SeedSamples({MakeSample("S-003", "WaferC", 0)});
    // 대기 수요(RESERVED)가 전혀 없어도 재고 0이면 고갈이어야 한다.
    SeedOrders({});

    MonitorService service(orderRepo, sampleRepo, queueRepo);
    const auto status = service.inventoryStatus();

    ASSERT_EQ(status.size(), 1u);
    EXPECT_EQ(status[0].pendingDemand, 0);
    EXPECT_EQ(status[0].level, InventoryLevel::DEPLETED);
}

// TC-P5-013 (잠정 채택, abnormal.md 1번): 재고==대기수요면 여유(전량 충당 가능).
TEST_F(MonitorServiceTest, InventoryStatus_SufficientWhenStockExactlyMatchesDemand) {
    SeedSamples({MakeSample("S-004", "WaferD", 7)});
    SeedOrders({MakeOrder("ORD-001", "S-004", OrderStatus::RESERVED, 7)});

    MonitorService service(orderRepo, sampleRepo, queueRepo);
    const auto status = service.inventoryStatus();

    ASSERT_EQ(status.size(), 1u);
    EXPECT_EQ(status[0].level, InventoryLevel::SUFFICIENT);
}

// TC-P5-014 (design.md §8 핵심 확정 — CLAUDE.md 지시대로 다르게 해석하지 않음):
// CONFIRMED/PRODUCING 주문 수량은 대기 수요에 절대 합산되지 않는다.
TEST_F(MonitorServiceTest, PendingDemand_ExcludesConfirmedAndProducingQuantities) {
    SeedSamples({MakeSample("S-005", "WaferE", 5)});
    SeedOrders({
        MakeOrder("ORD-001", "S-005", OrderStatus::RESERVED, 2),
        MakeOrder("ORD-002", "S-005", OrderStatus::CONFIRMED, 100),
        MakeOrder("ORD-003", "S-005", OrderStatus::PRODUCING, 200),
    });

    MonitorService service(orderRepo, sampleRepo, queueRepo);
    const auto status = service.inventoryStatus();

    ASSERT_EQ(status.size(), 1u);
    EXPECT_EQ(status[0].pendingDemand, 2)
        << "CONFIRMED/PRODUCING 수량까지 합산하면 이중 계산 회귀(design.md §8 위반)";
    EXPECT_EQ(status[0].level, InventoryLevel::SUFFICIENT);
}

// TC-P5-015: REJECTED 주문 수량도 대기 수요에 포함되지 않는다.
TEST_F(MonitorServiceTest, PendingDemand_ExcludesRejectedQuantity) {
    SeedSamples({MakeSample("S-006", "WaferF", 5)});
    SeedOrders({MakeOrder("ORD-001", "S-006", OrderStatus::REJECTED, 50)});

    MonitorService service(orderRepo, sampleRepo, queueRepo);
    const auto status = service.inventoryStatus();

    ASSERT_EQ(status.size(), 1u);
    EXPECT_EQ(status[0].pendingDemand, 0);
    EXPECT_EQ(status[0].level, InventoryLevel::SUFFICIENT);
}

// TC-P5-016: 여러 시료의 재고 현황이 서로 영향을 주지 않고 독립적으로 계산된다.
TEST_F(MonitorServiceTest, InventoryStatus_ComputesEachSampleIndependently) {
    SeedSamples({
        MakeSample("S-001", "WaferA", 10),
        MakeSample("S-002", "WaferB", 3),
        MakeSample("S-003", "WaferC", 0),
    });
    SeedOrders({
        MakeOrder("ORD-001", "S-001", OrderStatus::RESERVED, 5),
        MakeOrder("ORD-002", "S-002", OrderStatus::RESERVED, 10),
    });

    MonitorService service(orderRepo, sampleRepo, queueRepo);
    const auto status = service.inventoryStatus();

    ASSERT_EQ(status.size(), 3u);
    for (const auto& s : status) {
        if (s.sampleId == "S-001") {
            EXPECT_EQ(s.pendingDemand, 5);
            EXPECT_EQ(s.level, InventoryLevel::SUFFICIENT);
        } else if (s.sampleId == "S-002") {
            EXPECT_EQ(s.pendingDemand, 10);
            EXPECT_EQ(s.level, InventoryLevel::SHORTAGE);
        } else if (s.sampleId == "S-003") {
            EXPECT_EQ(s.pendingDemand, 0);
            EXPECT_EQ(s.level, InventoryLevel::DEPLETED);
        } else {
            FAIL() << "예상치 못한 sampleId: " << s.sampleId;
        }
    }
}

// ---- C. 메인 메뉴 요약 정보 (TC-P5-020~023) ----

// TC-P5-020: 등록 시료 종수/총 재고 합계.
TEST_F(MonitorServiceTest, Summary_SampleCountAndTotalStock) {
    SeedSamples({
        MakeSample("S-001", "WaferA", 10),
        MakeSample("S-002", "WaferB", 0),
        MakeSample("S-003", "WaferC", 5),
    });

    MonitorService service(orderRepo, sampleRepo, queueRepo);
    const auto summary = service.summary();

    EXPECT_EQ(summary.sampleCount, 3);
    EXPECT_EQ(summary.totalStock, 15);
}

// TC-P5-021 (잠정 채택, abnormal.md 2번): 전체 주문 건수는 REJECTED를 포함한 전체 레코드 수.
TEST_F(MonitorServiceTest, Summary_TotalOrderCount_IncludesRejected) {
    SeedOrders({
        MakeOrder("ORD-001", "S-001", OrderStatus::RESERVED, 1),
        MakeOrder("ORD-002", "S-001", OrderStatus::CONFIRMED, 1),
        MakeOrder("ORD-003", "S-001", OrderStatus::PRODUCING, 1),
        MakeOrder("ORD-004", "S-001", OrderStatus::RELEASE, 1),
        MakeOrder("ORD-005", "S-001", OrderStatus::REJECTED, 1),
    });

    MonitorService service(orderRepo, sampleRepo, queueRepo);

    EXPECT_EQ(service.summary().totalOrderCount, 5);
}

// TC-P5-022 (잠정 채택, abnormal.md 3번): 생산라인 대기 건수는 큐 전체 크기(현재 처리 중 포함).
TEST_F(MonitorServiceTest, Summary_ProductionQueueCount_IsWholeQueueSize) {
    SeedQueue({
        MakeQueueEntry("ORD-001", "S-001", "2026-07-15 09:00:00"),  // 현재 처리 중
        MakeQueueEntry("ORD-002", "S-001"),                          // 대기
        MakeQueueEntry("ORD-003", "S-001"),                          // 대기
    });

    MonitorService service(orderRepo, sampleRepo, queueRepo);

    EXPECT_EQ(service.summary().productionQueueCount, 3);
}

// TC-P5-023: 시료/주문/생산큐가 여러 건이어도 요약 값이 정확히 합산된다(종합).
TEST_F(MonitorServiceTest, Summary_AggregatesAllCountsTogether) {
    SeedSamples({
        MakeSample("S-001", "WaferA", 10),
        MakeSample("S-002", "WaferB", 20),
    });
    SeedOrders({
        MakeOrder("ORD-001", "S-001", OrderStatus::RESERVED, 1),
        MakeOrder("ORD-002", "S-001", OrderStatus::CONFIRMED, 1),
        MakeOrder("ORD-003", "S-002", OrderStatus::PRODUCING, 1),
        MakeOrder("ORD-004", "S-002", OrderStatus::REJECTED, 1),
    });
    SeedQueue({
        MakeQueueEntry("ORD-003", "S-002", "2026-07-15 09:00:00"),
        MakeQueueEntry("ORD-005", "S-001"),
    });

    MonitorService service(orderRepo, sampleRepo, queueRepo);
    const auto summary = service.summary();

    EXPECT_EQ(summary.sampleCount, 2);
    EXPECT_EQ(summary.totalStock, 30);
    EXPECT_EQ(summary.totalOrderCount, 4);
    EXPECT_EQ(summary.productionQueueCount, 2);
}

// ---- D. 경계값 — 전체 저장소가 비어 있을 때 (TC-P5-030~032) ----

// TC-P5-030: 주문/시료/생산큐가 전부 비어 있으면 크래시 없이 0/빈 목록.
TEST_F(MonitorServiceTest, AllEmpty_ReturnsZerosAndEmptyLists_NoCrash) {
    MonitorService service(orderRepo, sampleRepo, queueRepo);

    const auto counts = service.countAllStatuses();
    for (auto status : {OrderStatus::RESERVED, OrderStatus::CONFIRMED, OrderStatus::PRODUCING, OrderStatus::RELEASE}) {
        const int count = counts.count(status) ? counts.at(status) : 0;
        EXPECT_EQ(count, 0);
    }
    EXPECT_TRUE(service.ordersByStatus(OrderStatus::RESERVED).empty());
    EXPECT_TRUE(service.inventoryStatus().empty());

    const auto summary = service.summary();
    EXPECT_EQ(summary.sampleCount, 0);
    EXPECT_EQ(summary.totalStock, 0);
    EXPECT_EQ(summary.totalOrderCount, 0);
    EXPECT_EQ(summary.productionQueueCount, 0);
}

// TC-P5-031: 시료는 있지만 주문이 하나도 없을 때 재고 현황은 여유/고갈로만 갈린다.
TEST_F(MonitorServiceTest, SamplesWithoutOrders_AllSufficientOrDepleted) {
    SeedSamples({
        MakeSample("S-001", "WaferA", 5),
        MakeSample("S-002", "WaferB", 0),
    });

    MonitorService service(orderRepo, sampleRepo, queueRepo);
    const auto status = service.inventoryStatus();

    ASSERT_EQ(status.size(), 2u);
    for (const auto& s : status) {
        EXPECT_EQ(s.pendingDemand, 0);
        if (s.sampleId == "S-001") {
            EXPECT_EQ(s.level, InventoryLevel::SUFFICIENT);
        } else {
            EXPECT_EQ(s.level, InventoryLevel::DEPLETED);
        }
    }
}

// TC-P5-032: 주문이 가리키는 시료가 저장소에 없을 때(데이터 정합성 경계) 유령 항목이 생기지 않는다.
TEST_F(MonitorServiceTest, OrderReferencingMissingSample_DoesNotCreatePhantomInventoryEntry) {
    SeedSamples({});  // S-999가 저장소에 없음
    SeedOrders({MakeOrder("ORD-001", "S-999", OrderStatus::RESERVED, 5)});

    MonitorService service(orderRepo, sampleRepo, queueRepo);

    EXPECT_TRUE(service.inventoryStatus().empty());
}
