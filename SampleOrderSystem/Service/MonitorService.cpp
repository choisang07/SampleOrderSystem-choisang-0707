#include "MonitorService.h"

MonitorService::MonitorService(IOrderRepository& orderRepo, ISampleRepository& sampleRepo,
                                IProductionQueueRepository& queueRepo)
    : orderRepo_(orderRepo), sampleRepo_(sampleRepo), queueRepo_(queueRepo) {
}

std::vector<Order> MonitorService::ordersByStatus(OrderStatus status) const {
    std::vector<Order> result;
    if (status == OrderStatus::REJECTED) {
        // REJECTED는 모니터링 대상이 아니다(requirement.md 5.5절, abnormal.md 4번).
        return result;
    }

    for (const auto& order : orderRepo_.findAll()) {
        if (order.status == status) {
            result.push_back(order);
        }
    }
    return result;
}

std::map<OrderStatus, int> MonitorService::countAllStatuses() const {
    std::map<OrderStatus, int> counts;
    for (const auto& order : orderRepo_.findAll()) {
        if (order.status == OrderStatus::REJECTED) {
            continue; // REJECTED는 집계에서 제외(requirement.md 5.5절).
        }
        counts[order.status]++;
    }
    return counts;
}

std::vector<SampleInventoryStatus> MonitorService::inventoryStatus() const {
    std::vector<SampleInventoryStatus> result;
    const auto samples = sampleRepo_.findAll();
    const auto orders = orderRepo_.findAll();

    for (const auto& sample : samples) {
        int pendingDemand = 0;
        // "대기 수요" = 해당 시료에 대한 RESERVED 주문 수량 합만 집계한다.
        // CONFIRMED/PRODUCING은 이미 재고가 반영되었으므로 제외(design.md §8 확정).
        for (const auto& order : orders) {
            if (order.sampleId == sample.id && order.status == OrderStatus::RESERVED) {
                pendingDemand += order.quantity;
            }
        }

        InventoryLevel level;
        if (sample.stock == 0) {
            // 재고 0은 대기 수요와 무관하게 무조건 고갈(requirement.md 5.5절, 최우선 판정).
            level = InventoryLevel::DEPLETED;
        } else if (sample.stock >= pendingDemand) {
            // stock == pendingDemand인 경계값도 "전량 충당 가능"으로 보아 여유로 분류
            // (abnormal.md 1번 잠정 채택).
            level = InventoryLevel::SUFFICIENT;
        } else {
            level = InventoryLevel::SHORTAGE;
        }

        SampleInventoryStatus status;
        status.sampleId = sample.id;
        status.sampleName = sample.name;
        status.stock = sample.stock;
        status.pendingDemand = pendingDemand;
        status.level = level;
        result.push_back(status);
    }

    return result;
}

MonitoringSummary MonitorService::summary() const {
    MonitoringSummary summary;

    const auto samples = sampleRepo_.findAll();
    summary.sampleCount = static_cast<int>(samples.size());
    for (const auto& sample : samples) {
        summary.totalStock += sample.stock;
    }

    // 전체 주문 건수는 REJECTED를 포함한 저장소의 모든 레코드 수(abnormal.md 2번 잠정 채택).
    summary.totalOrderCount = static_cast<int>(orderRepo_.findAll().size());

    // 생산라인 대기 건수는 큐 전체 크기(현재 처리 중인 head 포함, abnormal.md 3번 잠정 채택).
    summary.productionQueueCount = static_cast<int>(queueRepo_.findAll().size());

    return summary;
}
