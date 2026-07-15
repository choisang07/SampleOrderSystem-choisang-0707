#pragma once

#include "IOrderState.h"

// PRODUCING 상태. 생산완료 처리(completeProduction)만 실제로 동작한다.
class ProducingState : public IOrderState {
public:
    static IOrderState& instance();

    IOrderState& approve(Order& order, SampleService& sampleService,
                          ProductionService& productionService) const override;
    IOrderState& reject(Order& order) const override;
    IOrderState& completeProduction(Order& order, SampleService& sampleService) const override;
    IOrderState& release(Order& order) const override;
    OrderStatus statusCode() const override;

private:
    ProducingState() = default;
};
