#pragma once

#include <vector>

#include "../Persistence/JsonFileStore.h"
#include "IOrderRepository.h"

// store.json 기반 구현체(design.md §3, §4.2).
class JsonOrderRepository : public IOrderRepository {
public:
    explicit JsonOrderRepository(JsonFileStore& store);

    std::optional<Order> findById(const std::string& id) const override;
    std::vector<Order> findAll() const override;
    void save(const Order& order) override;

private:
    void persist();

    JsonFileStore& store_;
    std::vector<Order> orders_;
};
