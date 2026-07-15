#pragma once

#include "gmock/gmock.h"

#include "../../Repository/ISampleRepository.h"

// design.md §4.2/§11.1: Service/Factory는 인터페이스에만 의존하므로,
// 파일 I/O 없이 순수 인메모리 스텁으로 대체할 수 있다.
class MockSampleRepository : public ISampleRepository {
public:
    MOCK_METHOD(std::optional<Sample>, findById, (const std::string& id), (const, override));
    MOCK_METHOD(std::vector<Sample>, findAll, (), (const, override));
    MOCK_METHOD(void, save, (const Sample& sample), (override));
};
