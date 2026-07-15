// Phase 1 유닛 테스트 — SampleService (docs_temp/Phase1/testcase.md TC-P1-001, 009~012, 020~025 대응)
//
// SampleService는 아직 Develope가 구현하지 않았다(design.md §3 기준으로 아래 시그니처를 가정).
//   class SampleService {
//   public:
//       explicit SampleService(ISampleRepository& repo);
//       Sample registerSample(const std::string& name, double avgProductionTime, double yield);
//       std::vector<Sample> listAll() const;
//       std::vector<Sample> search(const std::string& keyword) const;
//       // keyword가 빈 문자열이면 std::invalid_argument throw (확정: 재입력 요구).
//       // 매칭 규칙(확정): 이름(name) 대상, 부분 일치(contains), 대소문자 무시. ID는 대상 아님.
//       // ID 채번 포맷(확정): S-{3자리 순번} (예: S-001).
//   };
// 실제 시그니처가 다르면 Develope 구현 후 tester가 이 파일을 갱신한다.

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "../Service/SampleService.h"
#include "Mocks/MockSampleRepository.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::UnorderedElementsAre;

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

std::vector<std::string> IdsOf(const std::vector<Sample>& samples) {
    std::vector<std::string> ids;
    ids.reserve(samples.size());
    for (const auto& s : samples) {
        ids.push_back(s.id);
    }
    return ids;
}

}  // namespace

// ---- A. 등록 (TC-P1-001, TC-P1-009) ----

// TC-P1-001: 정상 등록 시 save가 호출되고 stock=0으로 초기화된다.
TEST(SampleServiceTest, RegisterSample_SavesWithAutoIdAndZeroStock) {
    NiceMock<MockSampleRepository> repo;
    ON_CALL(repo, findAll()).WillByDefault(Return(std::vector<Sample>{}));

    Sample saved;
    EXPECT_CALL(repo, save(_)).WillOnce([&](const Sample& s) { saved = s; });

    SampleService service(repo);
    const Sample result = service.registerSample("WaferA", 2.0, 0.9);

    EXPECT_EQ(result.id, "S-001");
    EXPECT_EQ(result.stock, 0);
    EXPECT_EQ(saved.id, "S-001");
    EXPECT_EQ(saved.name, "WaferA");
    EXPECT_EQ(saved.stock, 0);
}

// TC-P1-009 (확정): 이름 중복 등록도 허용되어 save가 호출된다.
TEST(SampleServiceTest, RegisterSample_AllowsDuplicateName) {
    NiceMock<MockSampleRepository> repo;
    ON_CALL(repo, findAll())
        .WillByDefault(Return(std::vector<Sample>{MakeSample("S-001", "WaferA")}));

    EXPECT_CALL(repo, save(_)).Times(1);

    SampleService service(repo);
    const Sample result = service.registerSample("WaferA", 3.0, 0.7);

    EXPECT_EQ(result.id, "S-002");
    EXPECT_EQ(result.name, "WaferA");
}

// 유효성 위반은 SampleFactory로 위임되어 save가 호출되지 않아야 한다.
TEST(SampleServiceTest, RegisterSample_RejectsInvalidYield_DoesNotSave) {
    NiceMock<MockSampleRepository> repo;
    ON_CALL(repo, findAll()).WillByDefault(Return(std::vector<Sample>{}));

    EXPECT_CALL(repo, save(_)).Times(0);

    SampleService service(repo);

    EXPECT_THROW(service.registerSample("WaferA", 2.0, 0.0), std::invalid_argument);
}

// ---- B. 조회 (TC-P1-010, 011, 012) ----

// TC-P1-010: 저장소가 비어 있으면 빈 목록 반환.
TEST(SampleServiceTest, ListAll_ReturnsEmpty_WhenNoSamples) {
    NiceMock<MockSampleRepository> repo;
    ON_CALL(repo, findAll()).WillByDefault(Return(std::vector<Sample>{}));

    SampleService service(repo);

    EXPECT_TRUE(service.listAll().empty());
}

// TC-P1-011: 재고가 있는 시료(Phase 3 반영 가정 픽스처)와 재고 0 시료가 함께 조회된다.
TEST(SampleServiceTest, ListAll_ReturnsAllSamplesIncludingStock) {
    NiceMock<MockSampleRepository> repo;
    ON_CALL(repo, findAll())
        .WillByDefault(Return(std::vector<Sample>{
            MakeSample("S-001", "WaferA", /*stock=*/5),
            MakeSample("S-002", "WaferB", /*stock=*/0),
        }));

    SampleService service(repo);
    const auto result = service.listAll();

    ASSERT_EQ(result.size(), 2u);
    EXPECT_THAT(IdsOf(result), UnorderedElementsAre("S-001", "S-002"));
}

// TC-P1-012: 등록 직후(stock=0)인 시료도 목록에서 제외되지 않는다.
TEST(SampleServiceTest, ListAll_IncludesZeroStockSample) {
    NiceMock<MockSampleRepository> repo;
    ON_CALL(repo, findAll())
        .WillByDefault(Return(std::vector<Sample>{MakeSample("S-002", "WaferB", /*stock=*/0)}));

    SampleService service(repo);
    const auto result = service.listAll();

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].id, "S-002");
    EXPECT_EQ(result[0].stock, 0);
}

// ---- C. 검색 (TC-P1-020~025) ----

class SampleServiceSearchTest : public ::testing::Test {
protected:
    NiceMock<MockSampleRepository> repo;

    void SetUp() override {
        ON_CALL(repo, findAll())
            .WillByDefault(Return(std::vector<Sample>{
                MakeSample("S-001", "WaferA"),
                MakeSample("S-002", "WaferB"),
            }));
    }
};

// TC-P1-020: 정확히 일치하는 이름 검색.
TEST_F(SampleServiceSearchTest, FindsExactNameMatch) {
    SampleService service(repo);

    const auto result = service.search("WaferA");

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].id, "S-001");
}

// TC-P1-021 (확정: 부분 일치): "Wafer"로 검색하면 S-001, S-002 모두 반환.
TEST_F(SampleServiceSearchTest, PartialMatch_ReturnsAllContainingSubstring) {
    SampleService service(repo);

    const auto result = service.search("Wafer");

    EXPECT_THAT(IdsOf(result), UnorderedElementsAre("S-001", "S-002"));
}

// TC-P1-022 (확정: 대소문자 무시): 소문자 검색어로도 매칭.
TEST_F(SampleServiceSearchTest, CaseInsensitiveMatch) {
    SampleService service(repo);

    const auto result = service.search("wafera");

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].id, "S-001");
}

// TC-P1-023 (확정: 재입력 요구): 빈 검색어는 예외를 던진다(검색 실행 안 함).
TEST_F(SampleServiceSearchTest, EmptyKeyword_Throws) {
    SampleService service(repo);

    EXPECT_THROW(service.search(""), std::invalid_argument);
}

// TC-P1-023 (확정: 재입력 요구): 공백만으로 이루어진 검색어도 빈 검색어로 간주해 예외를 던진다
// (SampleFactory::create의 이름 검증과 동일한 기준으로 통일).
TEST_F(SampleServiceSearchTest, BlankKeyword_Throws) {
    SampleService service(repo);

    EXPECT_THROW(service.search(" "), std::invalid_argument);
}

// TC-P1-024: 존재하지 않는 이름으로 검색하면 빈 목록.
TEST_F(SampleServiceSearchTest, NoMatch_ReturnsEmpty) {
    SampleService service(repo);

    EXPECT_TRUE(service.search("NotExist").empty());
}

// TC-P1-025 (확정: 이름만 대상, ID 제외): ID 값으로 검색해도 결과 없음.
TEST_F(SampleServiceSearchTest, SearchById_ReturnsEmpty_BecauseNameOnly) {
    SampleService service(repo);

    EXPECT_TRUE(service.search("S-001").empty());
}
