#pragma once

#include "IOrderState.h"

// REJECTED 상태. 정상 흐름 밖의 최종 상태이며 어떤 전이도 허용하지 않는다.
class RejectedState : public IOrderState {
public:
    static IOrderState& instance();

    IOrderState& approve(Order& order, SampleService& sampleService,
                          ProductionService& productionService) const override;
    IOrderState& reject(Order& order) const override;
    IOrderState& completeProduction(Order& order, SampleService& sampleService) const override;
    IOrderState& release(Order& order) const override;
    OrderStatus statusCode() const override;

private:
    RejectedState() = default;
};
