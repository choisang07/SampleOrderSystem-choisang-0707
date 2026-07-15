#include "JsonOrderRepository.h"

#include <algorithm>

JsonOrderRepository::JsonOrderRepository(JsonFileStore& store)
    : store_(store), orders_(store_.loadOrders()) {
}

std::optional<Order> JsonOrderRepository::findById(const std::string& id) const {
    const auto it = std::find_if(orders_.begin(), orders_.end(),
        [&](const Order& o) { return o.id == id; });
    if (it == orders_.end()) {
        return std::nullopt;
    }
    return *it;
}

std::vector<Order> JsonOrderRepository::findAll() const {
    return orders_;
}

void JsonOrderRepository::save(const Order& order) {
    const auto it = std::find_if(orders_.begin(), orders_.end(),
        [&](const Order& o) { return o.id == order.id; });
    if (it == orders_.end()) {
        orders_.push_back(order);
    } else {
        *it = order;
    }
    persist();
}

void JsonOrderRepository::persist() {
    store_.saveOrders(orders_);
}
