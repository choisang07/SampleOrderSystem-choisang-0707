#include "JsonProductionQueueRepository.h"

#include <algorithm>

JsonProductionQueueRepository::JsonProductionQueueRepository(JsonFileStore& store)
    : store_(store), queue_(store_.loadProductionQueue()) {
}

std::optional<ProductionQueueEntry> JsonProductionQueueRepository::findById(const std::string& orderId) const {
    const auto it = std::find_if(queue_.begin(), queue_.end(),
        [&](const ProductionQueueEntry& e) { return e.orderId == orderId; });
    if (it == queue_.end()) {
        return std::nullopt;
    }
    return *it;
}

std::vector<ProductionQueueEntry> JsonProductionQueueRepository::findAll() const {
    return queue_;
}

void JsonProductionQueueRepository::save(const ProductionQueueEntry& entry) {
    const auto it = std::find_if(queue_.begin(), queue_.end(),
        [&](const ProductionQueueEntry& e) { return e.orderId == entry.orderId; });
    if (it == queue_.end()) {
        queue_.push_back(entry); // 신규 항목은 큐 끝(FIFO)에 추가
    } else {
        *it = entry;
    }
    persist();
}

void JsonProductionQueueRepository::remove(const std::string& orderId) {
    const auto it = std::find_if(queue_.begin(), queue_.end(),
        [&](const ProductionQueueEntry& e) { return e.orderId == orderId; });
    if (it == queue_.end()) {
        return;
    }
    queue_.erase(it);
    persist();
}

void JsonProductionQueueRepository::persist() {
    store_.saveProductionQueue(queue_);
}
