#pragma once

#include <string>

#include "../../Model/Order.h"
#include "../../Repository/ISampleRepository.h"

// 주문 생성 검증 + 주문번호(ID) 자동 채번을 전담한다(design.md §4.4 Factory 패턴).
// Service/Controller는 이 Factory를 통해서만 신규 Order를 만든다.
//
// Phase 2 경계(CLAUDE.md 3절, docs_temp/Phase2/testcase.md 참고): 여기서는 절대 재고를
// 확인/차감하지 않는다. "등록된 시료인지" 존재 여부만 확인하고, 그 외 재고/생산 큐 관련
// 판단은 전혀 하지 않는다(Phase 3 OrderService::approve의 책임).
class OrderFactory {
public:
    // 유효성 검증(시료 존재, 고객명 필수, 수량>0) 후 ID를 자동 채번(ORD-{YYYYMMDD}-{HHMMSS})해
    // RESERVED 상태의 Order를 생성한다. 검증 실패 시 std::invalid_argument를 던진다.
    static Order create(const std::string& sampleId, const std::string& customerName, int quantity,
                        const ISampleRepository& sampleRepo);
};
