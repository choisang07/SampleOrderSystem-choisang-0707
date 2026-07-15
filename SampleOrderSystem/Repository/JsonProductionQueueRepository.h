#pragma once

#include <vector>

#include "../Persistence/JsonFileStore.h"
#include "IProductionQueueRepository.h"

// store.json 기반 구현체(design.md §3, §4.2). 배열 순서가 곧 FIFO 순서이며,
// 진실 공급원은 항상 이 Repository/JSON이다(design.md §4.3).
class JsonProductionQueueRepository : public IProductionQueueRepository {
public:
    explicit JsonProductionQueueRepository(JsonFileStore& store);

    std::optional<ProductionQueueEntry> findById(const std::string& orderId) const override;
    std::vector<ProductionQueueEntry> findAll() const override;
    void save(const ProductionQueueEntry& entry) override;
    void remove(const std::string& orderId) override;

private:
    void persist();

    JsonFileStore& store_;
    std::vector<ProductionQueueEntry> queue_;
};
