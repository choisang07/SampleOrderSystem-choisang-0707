#include "ReservedState.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "ConfirmedState.h"
#include "ProducingState.h"
#include "RejectedState.h"
#include "../../Model/ProductionQueueEntry.h"
#include "../../Model/Sample.h"
#include "../../Service/ProductionService.h"
#include "../../Service/SampleService.h"

IOrderState& ReservedState::instance() {
    static ReservedState instance;
    return instance;
}

// 핵심 비즈니스 로직(requirement.md 5.6절 사례1/사례2, design.md §6.1):
// 1) 재고 확인 시 생산 큐는 절대 조회하지 않는다(Sample.stock의 "현재" 값만 사용).
// 2) 재고로 충당 가능한 만큼(min(stock, quantity))은 승인 즉시 차감한다.
// 3) 남은 부족분이 있으면 실 생산량 = ceil(부족분 / 수율)을 계산해 생산 큐에 등록하고
//    PRODUCING으로 전이한다(생산량에는 수율 손실을 재적용하지 않는다).
// 4) 부족분이 0이면 CONFIRMED로 바로 전이한다.
IOrderState& ReservedState::approve(Order& order, SampleService& sampleService,
                                     ProductionService& productionService) const {
    const auto sampleOpt = sampleService.findById(order.sampleId);
    if (!sampleOpt) {
        throw std::invalid_argument("등록되지 않은 시료입니다: " + order.sampleId);
    }
    const Sample sample = *sampleOpt;

    const int coveredByStock = std::min(sample.stock, order.quantity);
    if (coveredByStock > 0) {
        sampleService.adjustStock(sample.id, -coveredByStock);
    }

    const int shortfall = order.quantity - coveredByStock;
    if (shortfall <= 0) {
        order.status = OrderStatus::CONFIRMED;
        return ConfirmedState::instance();
    }

    const int requiredQuantity =
        static_cast<int>(std::ceil(static_cast<double>(shortfall) / sample.yield));

    ProductionQueueEntry entry;
    entry.orderId = order.id;
    entry.sampleId = order.sampleId;
    entry.requiredQuantity = requiredQuantity;
    entry.totalProductionMinutes = sample.avgProductionTime * requiredQuantity;
    productionService.enqueue(entry);

    order.status = OrderStatus::PRODUCING;
    return ProducingState::instance();
}

IOrderState& ReservedState::reject(Order& order) const {
    order.status = OrderStatus::REJECTED;
    return RejectedState::instance();
}

IOrderState& ReservedState::completeProduction(Order&, SampleService&) const {
    // design.md §4.1: ProducingState만 실제 동작, 나머지는 no-op(예외 아님, 확정).
    return ReservedState::instance();
}

IOrderState& ReservedState::release(Order&) const {
    throw std::logic_error("RESERVED 상태에서는 출고할 수 없습니다.");
}

OrderStatus ReservedState::statusCode() const {
    return OrderStatus::RESERVED;
}
