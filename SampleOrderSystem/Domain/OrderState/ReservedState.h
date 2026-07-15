#pragma once

#include "IOrderState.h"

// RESERVED 상태. 승인(approve)/거절(reject)이 모두 허용되는 유일한 상태다.
class ReservedState : public IOrderState {
public:
    static IOrderState& instance();

    IOrderState& approve(Order& order, SampleService& sampleService,
                          ProductionService& productionService) const override;
    IOrderState& reject(Order& order) const override;
    IOrderState& completeProduction(Order& order, SampleService& sampleService) const override;
    IOrderState& release(Order& order) const override;
    OrderStatus statusCode() const override;

private:
    ReservedState() = default;
};
