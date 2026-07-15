#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../Model/Order.h"

// Service는 이 인터페이스에만 의존한다(design.md §4.2 Repository 패턴 + DIP).
class IOrderRepository {
public:
    virtual ~IOrderRepository() = default;

    virtual std::optional<Order> findById(const std::string& id) const = 0;
    virtual std::vector<Order> findAll() const = 0;
    virtual void save(const Order& order) = 0; // 변경 시 즉시 영속화
};
