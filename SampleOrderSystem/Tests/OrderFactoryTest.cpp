// Phase 2 유닛 테스트 — OrderFactory (docs_temp/Phase2/testcase.md TC-P2-001~032 대응)
//
// OrderFactory는 아직 Develope가 구현하지 않았다(design.md §3, §4.4, §5 기준으로 아래 시그니처를 가정).
//   class OrderFactory {
//   public:
//       // 유효성 검증(시료 존재, 고객명 필수, 수량>0) 후 ID를 자동 생성(ORD-{YYYYMMDD}-{HHMMSS})해
//       // RESERVED 상태의 Order를 생성한다. 검증 실패 시 std::invalid_argument throw.
//       static Order create(const std::string& sampleId, const std::string& customerName, int quantity,
//                           const ISampleRepository& sampleRepo);
//   };
// 실제 시그니처가 다르면 Develope 구현 후 tester가 이 파일을 갱신한다.
//
// Phase 2는 예약(RESERVED) 생성만 다루며 재고 확인/차감은 절대 하지 않는다(CLAUDE.md 3절,
// docs_temp/Phase2/testcase.md 경계 설명 참고) — 이 파일의 모든 테스트는 재고(stock) 값과
// 무관하게 동작해야 함을 검증한다.

#include <regex>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "../Domain/Factory/OrderFactory.h"
#include "Mocks/MockSampleRepository.h"

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

// TC-P2-001: 등록된 시료 대상 정상 예약은 RESERVED 상태로 생성된다.
TEST(OrderFactoryTest, CreatesReservedOrderOnHappyPath) {
    NiceMock<MockSampleRepository> sampleRepo;
    ON_CALL(sampleRepo, findById("S-001")).WillByDefault(Return(MakeSample("S-001", "WaferA")));

    const Order order = OrderFactory::create("S-001", "삼성전자", 200, sampleRepo);

    EXPECT_EQ(order.sampleId, "S-001");
    EXPECT_EQ(order.customerName, "삼성전자");
    EXPECT_EQ(order.quantity, 200);
    EXPECT_EQ(order.status, OrderStatus::RESERVED);
}

// TC-P2-002: 재고가 0(부족)이어도 예약 자체는 성공한다 — Phase 2/3 경계 확인.
TEST(OrderFactoryTest, SucceedsRegardlessOfStockLevel) {
    NiceMock<MockSampleRepository> sampleRepo;
    ON_CALL(sampleRepo, findById("S-001"))
        .WillByDefault(Return(MakeSample("S-001", "WaferA", /*stock=*/0)));

    const Order order = OrderFactory::create("S-001", "A대학연구실", 9999, sampleRepo);

    EXPECT_EQ(order.quantity, 9999);
    EXPECT_EQ(order.status, OrderStatus::RESERVED);
}

// TC-P2-003: 주문 ID는 ORD-{YYYYMMDD}-{HHMMSS} 형식이다(실행 시각 의존이므로 정규식으로 검증).
TEST(OrderFactoryTest, GeneratesIdMatchingOrdTimestampFormat) {
    NiceMock<MockSampleRepository> sampleRepo;
    ON_CALL(sampleRepo, findById("S-001")).WillByDefault(Return(MakeSample("S-001", "WaferA")));

    const Order order = OrderFactory::create("S-001", "삼성전자", 10, sampleRepo);

    static const std::regex kIdPattern(R"(^ORD-\d{8}-\d{6}$)");
    EXPECT_TRUE(std::regex_match(order.id, kIdPattern))
        << "실제 id: " << order.id;
}

// TC-P2-004: createdAt이 빈 문자열이 아닌 현재 시각으로 채워진다.
TEST(OrderFactoryTest, FillsCreatedAtTimestamp) {
    NiceMock<MockSampleRepository> sampleRepo;
    ON_CALL(sampleRepo, findById("S-001")).WillByDefault(Return(MakeSample("S-001", "WaferA")));

    const Order order = OrderFactory::create("S-001", "삼성전자", 10, sampleRepo);

    EXPECT_FALSE(order.createdAt.empty());
}

// TC-P2-010: 존재하지 않는 시료 ID로는 예약할 수 없다.
TEST(OrderFactoryTest, RejectsUnknownSampleId) {
    NiceMock<MockSampleRepository> sampleRepo;
    ON_CALL(sampleRepo, findById("S-999")).WillByDefault(Return(std::nullopt));

    EXPECT_THROW(OrderFactory::create("S-999", "테스트", 10, sampleRepo), std::invalid_argument);
}

// TC-P2-011: 시료 ID가 빈 문자열이면 거부된다(존재하지 않는 시료로 취급되든, 별도 메시지든
// 동작은 "거부"로 동일 — abnormal.md 4번 항목 참고).
TEST(OrderFactoryTest, RejectsEmptySampleId) {
    NiceMock<MockSampleRepository> sampleRepo;
    ON_CALL(sampleRepo, findById("")).WillByDefault(Return(std::nullopt));

    EXPECT_THROW(OrderFactory::create("", "테스트", 10, sampleRepo), std::invalid_argument);
}

// TC-P2-020: 고객명이 빈 문자열이면 거부된다.
TEST(OrderFactoryTest, RejectsEmptyCustomerName) {
    NiceMock<MockSampleRepository> sampleRepo;
    ON_CALL(sampleRepo, findById("S-001")).WillByDefault(Return(MakeSample("S-001", "WaferA")));

    EXPECT_THROW(OrderFactory::create("S-001", "", 10, sampleRepo), std::invalid_argument);
}

// TC-P2-020: 고객명이 공백 문자만이어도 거부된다.
TEST(OrderFactoryTest, RejectsBlankCustomerName) {
    NiceMock<MockSampleRepository> sampleRepo;
    ON_CALL(sampleRepo, findById("S-001")).WillByDefault(Return(MakeSample("S-001", "WaferA")));

    EXPECT_THROW(OrderFactory::create("S-001", "   ", 10, sampleRepo), std::invalid_argument);
}

// TC-P2-030: 주문 수량 0은 거부된다.
TEST(OrderFactoryTest, RejectsZeroQuantity) {
    NiceMock<MockSampleRepository> sampleRepo;
    ON_CALL(sampleRepo, findById("S-001")).WillByDefault(Return(MakeSample("S-001", "WaferA")));

    EXPECT_THROW(OrderFactory::create("S-001", "삼성전자", 0, sampleRepo), std::invalid_argument);
}

// TC-P2-031: 주문 수량 음수는 거부된다.
TEST(OrderFactoryTest, RejectsNegativeQuantity) {
    NiceMock<MockSampleRepository> sampleRepo;
    ON_CALL(sampleRepo, findById("S-001")).WillByDefault(Return(MakeSample("S-001", "WaferA")));

    EXPECT_THROW(OrderFactory::create("S-001", "삼성전자", -5, sampleRepo), std::invalid_argument);
}

// TC-P2-032: 주문 수량 1(최소 양수 경계)은 허용된다.
TEST(OrderFactoryTest, AcceptsQuantityOne) {
    NiceMock<MockSampleRepository> sampleRepo;
    ON_CALL(sampleRepo, findById("S-001")).WillByDefault(Return(MakeSample("S-001", "WaferA")));

    const Order order = OrderFactory::create("S-001", "삼성전자", 1, sampleRepo);

    EXPECT_EQ(order.quantity, 1);
}
