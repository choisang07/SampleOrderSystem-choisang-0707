#include "RejectedState.h"

#include <stdexcept>

IOrderState& RejectedState::instance() {
    static RejectedState instance;
    return instance;
}

IOrderState& RejectedState::approve(Order&, SampleService&, ProductionService&) const {
    throw std::logic_error("이미 거절된 주문입니다.");
}

IOrderState& RejectedState::reject(Order&) const {
    throw std::logic_error("이미 거절된 주문입니다.");
}

IOrderState& RejectedState::completeProduction(Order&, SampleService&) const {
    // design.md §4.1: ProducingState만 실제 동작, 나머지는 no-op(예외 아님, 확정).
    return RejectedState::instance();
}

IOrderState& RejectedState::release(Order&) const {
    throw std::logic_error("거절된 주문은 출고할 수 없습니다.");
}

OrderStatus RejectedState::statusCode() const {
    return OrderStatus::REJECTED;
}
