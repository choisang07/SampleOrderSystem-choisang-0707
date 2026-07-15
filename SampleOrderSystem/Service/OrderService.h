#pragma once

#include <string>

#include "../Model/Order.h"
#include "../Repository/IOrderRepository.h"
#include "../Repository/ISampleRepository.h"

// 주문 접수(예약) 오케스트레이션(design.md §3). Controller는 이 Service를 통해서만
// 주문 데이터를 다룬다.
//
// Phase 2 경계(CLAUDE.md 3절): reserve()는 OrderFactory로 검증/생성을 위임한 뒤
// IOrderRepository::save만 호출한다. 재고 확인/차감(ISampleRepository::save)은
// 절대 하지 않는다 — 이는 Phase 3 OrderService::approve의 책임이다.
class OrderService {
public:
    OrderService(IOrderRepository& orderRepo, ISampleRepository& sampleRepo);

    // OrderFactory로 검증/생성을 위임한 뒤 저장한다. 유효성 위반 시
    // std::invalid_argument가 그대로 전파되며 save는 호출되지 않는다.
    Order reserve(const std::string& sampleId, const std::string& customerName, int quantity);

private:
    IOrderRepository& orderRepo_;
    ISampleRepository& sampleRepo_;
};
