#include "ReleaseService.h"

#include <stdexcept>

ReleaseService::ReleaseService(IOrderRepository& orderRepo) : orderRepo_(orderRepo) {
}

std::vector<Order> ReleaseService::listReleasable() const {
    std::vector<Order> result;
    for (const auto& order : orderRepo_.findAll()) {
        if (order.status == OrderStatus::CONFIRMED) {
            result.push_back(order);
        }
    }
    return result;
}

Order ReleaseService::release(const std::string& orderId) {
    const auto found = orderRepo_.findById(orderId);
    if (!found.has_value()) {
        throw std::invalid_argument("존재하지 않는 주문입니다. (ID: " + orderId + ")");
    }

    Order order = found.value();
    if (order.status != OrderStatus::CONFIRMED) {
        throw std::invalid_argument("CONFIRMED 상태의 주문만 출고할 수 있습니다.");
    }

    order.status = OrderStatus::RELEASE;
    orderRepo_.save(order);
    return order;
}
