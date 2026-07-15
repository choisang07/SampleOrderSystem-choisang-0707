// Phase 4 유닛 테스트 — ReleaseService (docs_temp/Phase4/testcase.md TC-P4-001~031 대응)
//
// ReleaseService는 아직 Develope가 구현하지 않았다(design.md §3, §4.1 기준으로 아래
// 시그니처를 가정 — 근거/대안은 docs_temp/Phase4/abnormal.md 1번 항목 참고).
//   class ReleaseService {
//   public:
//       explicit ReleaseService(IOrderRepository& orderRepo);
//       std::vector<Order> listReleasable() const;     // CONFIRMED 상태만 필터링
//       Order release(const std::string& orderId);
//       // 존재하지 않는 주문 / CONFIRMED가 아닌 주문에 대한 출고 시도는
//       // std::invalid_argument를 던지며 save()는 호출되지 않는다.
//   };
// 재고(ISampleRepository)에 대한 의존성을 의도적으로 갖지 않는다 — 출고는 순수 상태
// 전이(CONFIRMED -> RELEASE)이며 재고는 승인/생산완료 시점에 이미 반영되었다(requirement.md 5.6, 5.7절).
// 이 계약이 구조적으로 지켜지므로, 별도의 MockSampleRepository 없이도 "재고 미접촉"이
// 증명된다(테스트 파일에 ISampleRepository/MockSampleRepository를 포함하지 않는 것 자체가
// 그 증거다). Develope가 대안(ISampleRepository를 받되 미사용)을 채택하면 이 파일에
// MockSampleRepository 기반 Times(0) 검증을 추가해야 한다(abnormal.md 1번 항목 참고).
// 실제 시그니처가 다르면 Develope 구현 후 tester가 이 파일을 갱신한다.

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "../Service/ReleaseService.h"
#include "Mocks/MockOrderRepository.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::UnorderedElementsAre;

namespace {

Order MakeOrder(const std::string& id, OrderStatus status, const std::string& sampleId = "S-001",
                 int quantity = 150, const std::string& customerName = "고객A") {
    Order o;
    o.id = id;
    o.sampleId = sampleId;
    o.customerName = customerName;
    o.quantity = quantity;
    o.status = status;
    o.createdAt = "2026-07-15 10:00:00";
    return o;
}

std::vector<std::string> IdsOf(const std::vector<Order>& orders) {
    std::vector<std::string> ids;
    ids.reserve(orders.size());
    for (const auto& o : orders) {
        ids.push_back(o.id);
    }
    return ids;
}

}  // namespace

// ---- A. 정상 출고 (TC-P4-001~003) ----

// TC-P4-001: CONFIRMED 주문은 정상적으로 RELEASE로 전이되고 save가 1회 호출된다.
TEST(ReleaseServiceTest, Release_ConfirmedOrder_TransitionsToRelease) {
    NiceMock<MockOrderRepository> repo;
    const Order confirmed = MakeOrder("ORD-001", OrderStatus::CONFIRMED);
    ON_CALL(repo, findById("ORD-001")).WillByDefault(Return(confirmed));

    Order saved;
    EXPECT_CALL(repo, save(_)).WillOnce([&](const Order& o) { saved = o; });

    ReleaseService service(repo);
    const Order result = service.release("ORD-001");

    EXPECT_EQ(result.status, OrderStatus::RELEASE);
    EXPECT_EQ(result.id, "ORD-001");
    EXPECT_EQ(result.sampleId, "S-001");
    EXPECT_EQ(result.customerName, "고객A");
    EXPECT_EQ(result.quantity, 150);

    EXPECT_EQ(saved.id, "ORD-001");
    EXPECT_EQ(saved.status, OrderStatus::RELEASE);
}

// TC-P4-002: ReleaseService는 ISampleRepository에 대한 의존성이 없으므로(구조적 보장),
// 이 테스트 파일 어디에도 MockSampleRepository가 등장하지 않는다는 사실 자체가
// "출고 시 재고 미접촉"의 증거다. 아래는 그 문서화를 위한 회귀 앵커 테스트.
TEST(ReleaseServiceTest, Release_DoesNotTouchSampleStock_ByConstruction) {
    NiceMock<MockOrderRepository> repo;
    const Order confirmed = MakeOrder("ORD-001", OrderStatus::CONFIRMED, "S-001", /*quantity=*/150);
    ON_CALL(repo, findById("ORD-001")).WillByDefault(Return(confirmed));
    EXPECT_CALL(repo, save(_)).Times(1);

    // ReleaseService(IOrderRepository&) 생성자는 ISampleRepository를 받지 않는다.
    // (만약 Develope가 대안을 채택해 ISampleRepository를 인자로 추가한다면, 이 테스트를
    //  ISampleRepository/MockSampleRepository 기반의 save() Times(0) 검증으로 갱신해야
    //  한다 — abnormal.md 1번 항목 참고.)
    ReleaseService service(repo);
    const Order result = service.release("ORD-001");

    EXPECT_EQ(result.status, OrderStatus::RELEASE);
}

// TC-P4-003: 두 CONFIRMED 주문 중 지정한 하나만 출고되고 나머지는 영향받지 않는다.
TEST(ReleaseServiceTest, Release_OnlyAffectsTargetedOrder) {
    NiceMock<MockOrderRepository> repo;
    const Order first = MakeOrder("ORD-001", OrderStatus::CONFIRMED);
    ON_CALL(repo, findById("ORD-001")).WillByDefault(Return(first));

    EXPECT_CALL(repo, save(_)).WillOnce([&](const Order& o) {
        EXPECT_EQ(o.id, "ORD-001");
        EXPECT_EQ(o.status, OrderStatus::RELEASE);
    });

    ReleaseService service(repo);
    service.release("ORD-001");
}

// ---- B. 상태 가드 — CONFIRMED가 아닌 주문은 출고 거부 (TC-P4-010~013) ----

class ReleaseServiceGuardTest : public ::testing::TestWithParam<OrderStatus> {};

TEST_P(ReleaseServiceGuardTest, Release_RejectsNonConfirmedOrder) {
    NiceMock<MockOrderRepository> repo;
    const Order order = MakeOrder("ORD-010", GetParam());
    ON_CALL(repo, findById("ORD-010")).WillByDefault(Return(order));

    EXPECT_CALL(repo, save(_)).Times(0);

    ReleaseService service(repo);

    EXPECT_THROW(service.release("ORD-010"), std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(
    NonConfirmedStatuses,
    ReleaseServiceGuardTest,
    ::testing::Values(OrderStatus::RESERVED, OrderStatus::PRODUCING, OrderStatus::REJECTED,
                       OrderStatus::RELEASE));

// ---- C. 존재하지 않는 주문번호 (TC-P4-020) ----

TEST(ReleaseServiceTest, Release_NonExistentOrder_Throws) {
    NiceMock<MockOrderRepository> repo;
    ON_CALL(repo, findById("ORD-999")).WillByDefault(Return(std::nullopt));

    EXPECT_CALL(repo, save(_)).Times(0);

    ReleaseService service(repo);

    EXPECT_THROW(service.release("ORD-999"), std::invalid_argument);
}

// ---- D. 출고 가능 목록 조회 (TC-P4-030~031) ----

// TC-P4-030: CONFIRMED 상태 주문만 필터링되어 반환된다.
TEST(ReleaseServiceTest, ListReleasable_ReturnsOnlyConfirmedOrders) {
    NiceMock<MockOrderRepository> repo;
    ON_CALL(repo, findAll())
        .WillByDefault(Return(std::vector<Order>{
            MakeOrder("ORD-001", OrderStatus::RESERVED),
            MakeOrder("ORD-002", OrderStatus::PRODUCING),
            MakeOrder("ORD-003", OrderStatus::CONFIRMED),
            MakeOrder("ORD-004", OrderStatus::CONFIRMED),
            MakeOrder("ORD-005", OrderStatus::RELEASE),
            MakeOrder("ORD-006", OrderStatus::REJECTED),
        }));

    ReleaseService service(repo);
    const auto result = service.listReleasable();

    EXPECT_THAT(IdsOf(result), UnorderedElementsAre("ORD-003", "ORD-004"));
}

// TC-P4-031: CONFIRMED 주문이 없으면 빈 목록을 반환한다(크래시 없음).
TEST(ReleaseServiceTest, ListReleasable_ReturnsEmpty_WhenNoConfirmedOrders) {
    NiceMock<MockOrderRepository> repo;
    ON_CALL(repo, findAll())
        .WillByDefault(Return(std::vector<Order>{
            MakeOrder("ORD-001", OrderStatus::RESERVED),
            MakeOrder("ORD-005", OrderStatus::RELEASE),
        }));

    ReleaseService service(repo);

    EXPECT_TRUE(service.listReleasable().empty());
}

TEST(ReleaseServiceTest, ListReleasable_ReturnsEmpty_WhenRepositoryEmpty) {
    NiceMock<MockOrderRepository> repo;
    ON_CALL(repo, findAll()).WillByDefault(Return(std::vector<Order>{}));

    ReleaseService service(repo);

    EXPECT_TRUE(service.listReleasable().empty());
}
