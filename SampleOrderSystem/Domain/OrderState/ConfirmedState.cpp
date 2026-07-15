#include "ConfirmedState.h"

#include <stdexcept>

#include "ReleasedState.h"

IOrderState& ConfirmedState::instance() {
    static ConfirmedState instance;
    return instance;
}

IOrderState& ConfirmedState::approve(Order&, SampleService&, ProductionService&) const {
    throw std::logic_error("이미 처리(승인)된 주문입니다.");
}

IOrderState& ConfirmedState::reject(Order&) const {
    throw std::logic_error("이미 승인된 주문은 거절할 수 없습니다.");
}

IOrderState& ConfirmedState::completeProduction(Order&, SampleService&) const {
    // design.md §4.1: ProducingState만 실제 동작, 나머지는 no-op(예외 아님, 확정).
    return ConfirmedState::instance();
}

// Phase 4(ReleaseService)에서 실제로 사용된다: CONFIRMED -> RELEASE.
IOrderState& ConfirmedState::release(Order& order) const {
    order.status = OrderStatus::RELEASE;
    return ReleasedState::instance();
}

OrderStatus ConfirmedState::statusCode() const {
    return OrderStatus::CONFIRMED;
}
