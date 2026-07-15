#pragma once

#include "../../Model/Order.h"

// Service는 전방 선언만 참조한다(design.md §4.1). 실제 정의는 각 State .cpp에서 필요 시 include.
class SampleService;
class ProductionService;

// 주문 상태 전이(State 패턴, design.md §4.1). 각 구현체는 인스턴스 필드가 없는
// stateless singleton(§10 결정)이며, static instance()로만 얻는다.
class IOrderState {
public:
    virtual ~IOrderState() = default;

    // 승인. 재고 확인(생산 큐는 절대 조회하지 않음, requirement.md 5.6절 사례2) ->
    // 충당 가능분 즉시 차감 -> 부족분 있으면 생산 큐 등록 후 PRODUCING, 없으면 CONFIRMED.
    // 허용되지 않는 상태에서 호출하면 std::logic_error를 던진다(abnormal.md 1번 확정).
    virtual IOrderState& approve(Order& order, SampleService& sampleService,
                                  ProductionService& productionService) const = 0;

    // 거절. RESERVED에서만 허용된다(그 외에는 std::logic_error).
    virtual IOrderState& reject(Order& order) const = 0;

    // 생산 완료 처리. ProducingState에서만 실제로 동작(PRODUCING -> CONFIRMED)하고,
    // 그 외 상태는 명시적으로 no-op이다(design.md §4.1, 모호함 없음, 예외 아님).
    virtual IOrderState& completeProduction(Order& order, SampleService& sampleService) const = 0;

    // 출고. CONFIRMED에서만 허용된다(그 외에는 std::logic_error). Phase 4에서 실제로 사용된다.
    virtual IOrderState& release(Order& order) const = 0;

    virtual OrderStatus statusCode() const = 0;
};

// 현재 order.status에 대응하는 State 싱글턴을 반환한다. 매번 switch로 분기하지 않도록
// 공용 헬퍼로 제공한다(Service/Controller에서 재사용).
IOrderState& stateFor(OrderStatus status);
