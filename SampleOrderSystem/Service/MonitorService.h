#pragma once

#include <map>
#include <string>
#include <vector>

#include "../Model/Order.h"
#include "../Model/ProductionQueueEntry.h"
#include "../Model/Sample.h"
#include "../Repository/IOrderRepository.h"
#include "../Repository/IProductionQueueRepository.h"
#include "../Repository/ISampleRepository.h"

// 재고 판정 등급. requirement.md 5.5절("여유"/"부족"/"고갈") 참고.
enum class InventoryLevel { SUFFICIENT, SHORTAGE, DEPLETED };

// 시료별 재고 현황 항목(requirement.md 5.5절 "재고량 확인").
struct SampleInventoryStatus {
    std::string sampleId;
    std::string sampleName;
    int stock = 0;
    int pendingDemand = 0;  // 해당 시료에 대한 RESERVED 주문 수량 합 (design.md §8 확정)
    InventoryLevel level = InventoryLevel::SUFFICIENT;
};

// 메인 메뉴 상단 요약 정보(requirement.md 5.1절, docs/screens.md 메인 메뉴).
struct MonitoringSummary {
    int sampleCount = 0;          // 등록 시료 종수
    int totalStock = 0;           // 총 재고 합계
    int totalOrderCount = 0;      // 전체 주문 건수(REJECTED 포함, docs_temp/Phase5/abnormal.md 2번 잠정 채택)
    int productionQueueCount = 0; // 생산라인 대기 건수(큐 전체 크기, abnormal.md 3번 잠정 채택)
};

// 상태별 주문 현황 / 재고 현황 집계 전담(design.md §3, §6-5). 순수 조회 로직만
// 담당하며 저장된 값을 그대로 읽어 집계할 뿐 다른 Service의 불변식을 검증하지
// 않는다(design.md §4.2 Repository/DIP 원칙).
class MonitorService {
public:
    MonitorService(IOrderRepository& orderRepo, ISampleRepository& sampleRepo,
                   IProductionQueueRepository& queueRepo);

    // REJECTED 제외, 상태별 목록(requirement.md 5.5절 "주문량 확인").
    // OrderStatus::REJECTED로 호출하면 빈 목록을 반환한다(docs_temp/Phase5/abnormal.md 4번 잠정 채택).
    std::vector<Order> ordersByStatus(OrderStatus status) const;

    // RESERVED/CONFIRMED/PRODUCING/RELEASE 4개 상태의 건수 맵(REJECTED 키 없음).
    std::map<OrderStatus, int> countAllStatuses() const;

    // 시료별 재고 현황(requirement.md 5.5절 "재고량 확인").
    std::vector<SampleInventoryStatus> inventoryStatus() const;

    // 메인 메뉴 상단 요약 정보(requirement.md 5.1절, docs/screens.md 메인 메뉴).
    MonitoringSummary summary() const;

private:
    IOrderRepository& orderRepo_;
    ISampleRepository& sampleRepo_;
    IProductionQueueRepository& queueRepo_;
};
