#pragma once

#include "IOrderState.h"

// CONFIRMED 상태. 출고(release)만 허용된다.
class ConfirmedState : public IOrderState {
public:
    static IOrderState& instance();

    IOrderState& approve(Order& order, SampleService& sampleService,
                          ProductionService& productionService) const override;
    IOrderState& reject(Order& order) const override;
    IOrderState& completeProduction(Order& order, SampleService& sampleService) const override;
    IOrderState& release(Order& order) const override;
    OrderStatus statusCode() const override;

private:
    ConfirmedState() = default;
};
