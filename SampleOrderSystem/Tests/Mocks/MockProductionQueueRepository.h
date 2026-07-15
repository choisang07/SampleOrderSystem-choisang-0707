#pragma once

#include "gmock/gmock.h"

#include "../../Repository/IProductionQueueRepository.h"

// design.md §4.2/§10/§11.1: 단일 글로벌 FIFO 큐 Repository의 Mock.
// findAll()이 반환하는 벡터의 순서 자체가 FIFO 순서(0번째 = head)라는 계약을
// 그대로 유지해 스텁한다.
class MockProductionQueueRepository : public IProductionQueueRepository {
public:
    MOCK_METHOD(std::optional<ProductionQueueEntry>, findById, (const std::string& orderId), (const, override));
    MOCK_METHOD(std::vector<ProductionQueueEntry>, findAll, (), (const, override));
    MOCK_METHOD(void, save, (const ProductionQueueEntry& entry), (override));
    MOCK_METHOD(void, remove, (const std::string& orderId), (override));
};
