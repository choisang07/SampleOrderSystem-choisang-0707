#include "ProducingState.h"

#include <stdexcept>

#include "ConfirmedState.h"

IOrderState& ProducingState::instance() {
    static ProducingState instance;
    return instance;
}

IOrderState& ProducingState::approve(Order&, SampleService&, ProductionService&) const {
    throw std::logic_error("이미 생산 중인 주문입니다.");
}

IOrderState& ProducingState::reject(Order&) const {
    throw std::logic_error("생산 중인 주문은 거절할 수 없습니다.");
}

// 생산 완료 시점(ProductionService::completeActive)에만 호출된다. 실제 재고 반영은
// ProductionService가 생산 큐 항목(requiredQuantity)을 이용해 직접 수행하므로(수율 재적용 없음,
// 규칙 4), 여기서는 상태 전이(PRODUCING -> CONFIRMED)만 담당한다.
IOrderState& ProducingState::completeProduction(Order& order, SampleService&) const {
    order.status = OrderStatus::CONFIRMED;
    return ConfirmedState::instance();
}

IOrderState& ProducingState::release(Order&) const {
    throw std::logic_error("생산 중인 주문은 출고할 수 없습니다.");
}

OrderStatus ProducingState::statusCode() const {
    return OrderStatus::PRODUCING;
}
