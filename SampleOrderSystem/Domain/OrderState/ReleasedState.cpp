#include "ReleasedState.h"

#include <stdexcept>

IOrderState& ReleasedState::instance() {
    static ReleasedState instance;
    return instance;
}

IOrderState& ReleasedState::approve(Order&, SampleService&, ProductionService&) const {
    throw std::logic_error("이미 출고된 주문입니다.");
}

IOrderState& ReleasedState::reject(Order&) const {
    throw std::logic_error("이미 출고된 주문은 거절할 수 없습니다.");
}

IOrderState& ReleasedState::completeProduction(Order&, SampleService&) const {
    // design.md §4.1: ProducingState만 실제 동작, 나머지는 no-op(예외 아님, 확정).
    return ReleasedState::instance();
}

IOrderState& ReleasedState::release(Order&) const {
    throw std::logic_error("이미 출고된 주문입니다.");
}

OrderStatus ReleasedState::statusCode() const {
    return OrderStatus::RELEASE;
}
