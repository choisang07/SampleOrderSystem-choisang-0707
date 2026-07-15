#include "IOrderState.h"

#include <stdexcept>

#include "ConfirmedState.h"
#include "ProducingState.h"
#include "RejectedState.h"
#include "ReleasedState.h"
#include "ReservedState.h"

IOrderState& stateFor(OrderStatus status) {
    switch (status) {
        case OrderStatus::RESERVED:  return ReservedState::instance();
        case OrderStatus::PRODUCING: return ProducingState::instance();
        case OrderStatus::CONFIRMED: return ConfirmedState::instance();
        case OrderStatus::RELEASE:   return ReleasedState::instance();
        case OrderStatus::REJECTED:  return RejectedState::instance();
    }
    throw std::logic_error("알 수 없는 주문 상태입니다.");
}
