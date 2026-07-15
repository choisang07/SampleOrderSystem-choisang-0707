// Phase 1 유닛 테스트 — SampleFactory (docs_temp/Phase1/testcase.md TC-P1-001~009 대응)
//
// SampleFactory는 아직 Develope가 구현하지 않았다(design.md §3, §4.4 기준으로 아래 시그니처를 가정).
//   class SampleFactory {
//   public:
//       static Sample create(const std::string& name, double avgProductionTime, double yield,
//                             const ISampleRepository& repo);
//       // 유효성 위반 시 std::invalid_argument throw.
//       // id는 repo.findAll()의 기존 ID(S-{3자리 순번}) 중 최댓값 + 1로 자동 채번, 3자리 0패딩
//       // (확정: docs_temp/Phase1/abnormal.md 1번 — design.md §5/§8, screens.md 200행 기준).
//   };
// 실제 시그니처가 다르면 Develope 구현 후 tester가 이 파일을 갱신한다.

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "../Domain/Factory/SampleFactory.h"
#include "Mocks/MockSampleRepository.h"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

Sample MakeSample(const std::string& id, const std::string& name) {
    Sample s;
    s.id = id;
    s.name = name;
    s.avgProductionTime = 1.0;
    s.yield = 0.9;
    s.stock = 0;
    return s;
}

}  // namespace

// TC-P1-001 / TC-P1-002b: 저장소가 비어 있으면 최초 ID는 "S-001".
TEST(SampleFactoryTest, AssignsFirstIdWhenRepositoryEmpty) {
    NiceMock<MockSampleRepository> repo;
    ON_CALL(repo, findAll()).WillByDefault(Return(std::vector<Sample>{}));

    const Sample sample = SampleFactory::create("WaferA", 2.0, 0.9, repo);

    EXPECT_EQ(sample.id, "S-001");
    EXPECT_EQ(sample.name, "WaferA");
    EXPECT_DOUBLE_EQ(sample.avgProductionTime, 2.0);
    EXPECT_DOUBLE_EQ(sample.yield, 0.9);
    EXPECT_EQ(sample.stock, 0);  // 확정: 등록 입력값에 stock 없음, 항상 0으로 초기화.
}

// TC-P1-002b: 기존에 S-001~S-003이 있으면 다음 ID는 "S-004"(순번 최댓값+1, 3자리 0패딩).
TEST(SampleFactoryTest, AssignsNextIncrementedId) {
    NiceMock<MockSampleRepository> repo;
    ON_CALL(repo, findAll())
        .WillByDefault(Return(std::vector<Sample>{
            MakeSample("S-001", "WaferA"),
            MakeSample("S-002", "WaferB"),
            MakeSample("S-003", "WaferC"),
        }));

    const Sample sample = SampleFactory::create("WaferD", 1.0, 0.8, repo);

    EXPECT_EQ(sample.id, "S-004");
}

// TC-P1-003: 이름이 빈 문자열이면 거부.
TEST(SampleFactoryTest, RejectsEmptyName) {
    NiceMock<MockSampleRepository> repo;
    ON_CALL(repo, findAll()).WillByDefault(Return(std::vector<Sample>{}));

    EXPECT_THROW(SampleFactory::create("", 2.0, 0.9, repo), std::invalid_argument);
}

// TC-P1-003: 이름이 공백만이어도 거부.
TEST(SampleFactoryTest, RejectsBlankName) {
    NiceMock<MockSampleRepository> repo;
    ON_CALL(repo, findAll()).WillByDefault(Return(std::vector<Sample>{}));

    EXPECT_THROW(SampleFactory::create("   ", 2.0, 0.9, repo), std::invalid_argument);
}

// TC-P1-004: 평균 생산시간 0은 거부(0 초과 조건).
TEST(SampleFactoryTest, RejectsZeroAvgProductionTime) {
    NiceMock<MockSampleRepository> repo;
    ON_CALL(repo, findAll()).WillByDefault(Return(std::vector<Sample>{}));

    EXPECT_THROW(SampleFactory::create("WaferA", 0.0, 0.9, repo), std::invalid_argument);
}

// TC-P1-004: 평균 생산시간 음수는 거부.
TEST(SampleFactoryTest, RejectsNegativeAvgProductionTime) {
    NiceMock<MockSampleRepository> repo;
    ON_CALL(repo, findAll()).WillByDefault(Return(std::vector<Sample>{}));

    EXPECT_THROW(SampleFactory::create("WaferA", -1.5, 0.9, repo), std::invalid_argument);
}

// TC-P1-005 (확정: 가정 B): 수율 0.0은 거부. 유효 범위는 (0, 1].
TEST(SampleFactoryTest, RejectsZeroYield) {
    NiceMock<MockSampleRepository> repo;
    ON_CALL(repo, findAll()).WillByDefault(Return(std::vector<Sample>{}));

    EXPECT_THROW(SampleFactory::create("WaferA", 2.0, 0.0, repo), std::invalid_argument);
}

// TC-P1-006: 수율 1.0(상한 경계)은 허용.
TEST(SampleFactoryTest, AcceptsYieldOne) {
    NiceMock<MockSampleRepository> repo;
    ON_CALL(repo, findAll()).WillByDefault(Return(std::vector<Sample>{}));

    const Sample sample = SampleFactory::create("WaferA", 2.0, 1.0, repo);

    EXPECT_DOUBLE_EQ(sample.yield, 1.0);
}

// TC-P1-007: 수율 음수는 거부.
TEST(SampleFactoryTest, RejectsNegativeYield) {
    NiceMock<MockSampleRepository> repo;
    ON_CALL(repo, findAll()).WillByDefault(Return(std::vector<Sample>{}));

    EXPECT_THROW(SampleFactory::create("WaferA", 2.0, -0.1, repo), std::invalid_argument);
}

// TC-P1-007: 수율 1 초과는 거부.
TEST(SampleFactoryTest, RejectsYieldAboveOne) {
    NiceMock<MockSampleRepository> repo;
    ON_CALL(repo, findAll()).WillByDefault(Return(std::vector<Sample>{}));

    EXPECT_THROW(SampleFactory::create("WaferA", 2.0, 1.5, repo), std::invalid_argument);
}

// TC-P1-009 (확정): 이름 중복은 Factory 레벨에서 막지 않는다(유일성 검증 대상은 ID뿐).
TEST(SampleFactoryTest, AllowsDuplicateName) {
    NiceMock<MockSampleRepository> repo;
    ON_CALL(repo, findAll())
        .WillByDefault(Return(std::vector<Sample>{MakeSample("S-001", "WaferA")}));

    const Sample sample = SampleFactory::create("WaferA", 3.0, 0.7, repo);

    EXPECT_EQ(sample.id, "S-002");
    EXPECT_EQ(sample.name, "WaferA");
}
