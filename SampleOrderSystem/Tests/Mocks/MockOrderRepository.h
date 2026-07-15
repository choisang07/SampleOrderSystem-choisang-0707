#pragma once

#include "gmock/gmock.h"

#include "../../Repository/IOrderRepository.h"

// design.md §4.2/§11.1: Service/Factory는 인터페이스에만 의존하므로,
// 파일 I/O 없이 순수 인메모리 스텁으로 대체할 수 있다.
class MockOrderRepository : public IOrderRepository {
public:
    MOCK_METHOD(std::optional<Order>, findById, (const std::string& id), (const, override));
    MOCK_METHOD(std::vector<Order>, findAll, (), (const, override));
    MOCK_METHOD(void, save, (const Order& order), (override));
};
