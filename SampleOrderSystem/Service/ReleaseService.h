#pragma once

#include <string>
#include <vector>

#include "../Model/Order.h"
#include "../Repository/IOrderRepository.h"

// 출고 처리 오케스트레이션(design.md §3, §4.1). CONFIRMED 상태의 주문을
// RELEASE로 전이하는 순수 상태 전이만 담당한다.
// 재고(ISampleRepository)는 승인/생산완료 시점에 이미 반영되었으므로
// (requirement.md 5.6, 5.7절) 이 Service는 재고 저장소에 대한 의존성을
// 의도적으로 갖지 않는다(docs_temp/Phase4/abnormal.md 1번 항목 참고).
class ReleaseService {
public:
    explicit ReleaseService(IOrderRepository& orderRepo);

    // CONFIRMED 상태 주문만 반환한다(출고 가능 목록).
    std::vector<Order> listReleasable() const;

    // orderId로 주문을 찾아 CONFIRMED 상태인 경우에만 RELEASE로 전이하고 저장한다.
    // - 주문이 없으면 std::invalid_argument
    // - CONFIRMED가 아니면 std::invalid_argument
    // - 성공 시 저장된(갱신된) Order를 반환한다.
    Order release(const std::string& orderId);

private:
    IOrderRepository& orderRepo_;
};
