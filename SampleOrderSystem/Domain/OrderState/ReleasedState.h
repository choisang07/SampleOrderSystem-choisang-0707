#pragma once

#include "IOrderState.h"

// RELEASE 상태. 정상 흐름의 마지막 상태이며 어떤 전이도 허용하지 않는다.
class ReleasedState : public IOrderState {
public:
    static IOrderState& instance();

    IOrderState& approve(Order& order, SampleService& sampleService,
                          ProductionService& productionService) const override;
    IOrderState& reject(Order& order) const override;
    IOrderState& completeProduction(Order& order, SampleService& sampleService) const override;
    IOrderState& release(Order& order) const override;
    OrderStatus statusCode() const override;

private:
    ReleasedState() = default;
};
