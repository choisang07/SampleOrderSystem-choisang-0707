// Phase 2 유닛 테스트 — OrderService (docs_temp/Phase2/testcase.md TC-P2-001~040 대응)
//
// OrderService는 아직 Develope가 구현하지 않았다(design.md §3, §9 기준으로 아래 시그니처를 가정).
//   class OrderService {
//   public:
//       OrderService(IOrderRepository& orderRepo, ISampleRepository& sampleRepo);
//       // OrderFactory로 검증/생성을 위임한 뒤 IOrderRepository::save로 저장한다.
//       // 재고 확인/차감은 하지 않는다(Phase 3 OrderService::approve의 책임).
//       Order reserve(const std::string& sampleId, const std::string& customerName, int quantity);
//   };
// 실제 시그니처가 다르면 Develope 구현 후 tester가 이 파일을 갱신한다.

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "../Service/OrderService.h"
#include "Mocks/MockOrderRepository.h"
#include "Mocks/MockSampleRepository.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

Sample MakeSample(const std::string& id, const std::string& name, int stock = 0) {
    Sample s;
    s.id = id;
    s.name = name;
    s.avgProductionTime = 1.0;
    s.yield = 0.9;
    s.stock = stock;
    return s;
}

}  // namespace

// TC-P2-001: 정상 예약 시 RESERVED 주문이 IOrderRepository::save로 저장된다.
TEST(OrderServiceTest, ReserveSavesReservedOrder) {
    NiceMock<MockSampleRepository> sampleRepo;
    MockOrderRepository orderRepo;
    ON_CALL(sampleRepo, findById("S-001")).WillByDefault(Return(MakeSample("S-001", "WaferA")));

    Order savedOrder;
    EXPECT_CALL(orderRepo, save(_))
        .Times(1)
        .WillOnce([&](const Order& order) { savedOrder = order; });

    OrderService service(orderRepo, sampleRepo);
    const Order result = service.reserve("S-001", "삼성전자", 200);

    EXPECT_EQ(result.status, OrderStatus::RESERVED);
    EXPECT_EQ(savedOrder.sampleId, "S-001");
    EXPECT_EQ(savedOrder.customerName, "삼성전자");
    EXPECT_EQ(savedOrder.quantity, 200);
    EXPECT_EQ(savedOrder.status, OrderStatus::RESERVED);
}

// TC-P2-002: 예약 처리 중 ISampleRepository::save(재고 변경)는 절대 호출되지 않는다
// — 재고 확인/차감은 Phase 3(승인)의 책임이며 Phase 2는 재고를 건드리지 않는다.
TEST(OrderServiceTest, ReserveNeverMutatesSampleStock) {
    NiceMock<MockSampleRepository> sampleRepo;
    NiceMock<MockOrderRepository> orderRepo;
    ON_CALL(sampleRepo, findById("S-001"))
        .WillByDefault(Return(MakeSample("S-001", "WaferA", /*stock=*/0)));

    EXPECT_CALL(sampleRepo, save(_)).Times(0);

    OrderService service(orderRepo, sampleRepo);
    service.reserve("S-001", "A대학연구실", 9999);  // 재고(0)보다 훨씬 큰 수량이어도 무관.
}

// TC-P2-010: 존재하지 않는 시료로는 예약할 수 없고, 이 경우 IOrderRepository::save는
// 호출되지 않는다(OrderFactory 예외가 Service까지 그대로 전파됨).
TEST(OrderServiceTest, ReserveWithUnknownSampleThrowsAndDoesNotSave) {
    NiceMock<MockSampleRepository> sampleRepo;
    MockOrderRepository orderRepo;
    ON_CALL(sampleRepo, findById("S-999")).WillByDefault(Return(std::nullopt));

    EXPECT_CALL(orderRepo, save(_)).Times(0);

    OrderService service(orderRepo, sampleRepo);
    EXPECT_THROW(service.reserve("S-999", "테스트", 10), std::invalid_argument);
}

// TC-P2-030: 주문 수량 0은 예약이 거부되고 저장도 되지 않는다.
TEST(OrderServiceTest, ReserveWithZeroQuantityThrowsAndDoesNotSave) {
    NiceMock<MockSampleRepository> sampleRepo;
    MockOrderRepository orderRepo;
    ON_CALL(sampleRepo, findById("S-001")).WillByDefault(Return(MakeSample("S-001", "WaferA")));

    EXPECT_CALL(orderRepo, save(_)).Times(0);

    OrderService service(orderRepo, sampleRepo);
    EXPECT_THROW(service.reserve("S-001", "삼성전자", 0), std::invalid_argument);
}

// TC-P2-020: 고객명이 빈 문자열이면 예약이 거부되고 저장도 되지 않는다.
TEST(OrderServiceTest, ReserveWithEmptyCustomerNameThrowsAndDoesNotSave) {
    NiceMock<MockSampleRepository> sampleRepo;
    MockOrderRepository orderRepo;
    ON_CALL(sampleRepo, findById("S-001")).WillByDefault(Return(MakeSample("S-001", "WaferA")));

    EXPECT_CALL(orderRepo, save(_)).Times(0);

    OrderService service(orderRepo, sampleRepo);
    EXPECT_THROW(service.reserve("S-001", "", 10), std::invalid_argument);
}

// TC-P2-040: 동일 시료에 대해 서로 다른 고객이 여러 건 예약하면, 각각 별개의 RESERVED
// 주문으로 저장된다(시료당 1건 제약 없음).
TEST(OrderServiceTest, AllowsMultipleReservationsForSameSample) {
    NiceMock<MockSampleRepository> sampleRepo;
    MockOrderRepository orderRepo;
    ON_CALL(sampleRepo, findById("S-001")).WillByDefault(Return(MakeSample("S-001", "WaferA")));

    EXPECT_CALL(orderRepo, save(_)).Times(2);

    OrderService service(orderRepo, sampleRepo);
    const Order first = service.reserve("S-001", "삼성전자", 200);
    const Order second = service.reserve("S-001", "LG이노텍", 50);

    EXPECT_EQ(first.status, OrderStatus::RESERVED);
    EXPECT_EQ(second.status, OrderStatus::RESERVED);
    EXPECT_EQ(first.customerName, "삼성전자");
    EXPECT_EQ(second.customerName, "LG이노텍");
}
