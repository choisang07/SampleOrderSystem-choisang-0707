#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../Model/ProductionQueueEntry.h"

// 단일 글로벌 FIFO 큐(design.md §10 결정). 배열 순서 자체가 FIFO 순서다.
// Service는 이 인터페이스에만 의존한다(design.md §4.2 Repository 패턴 + DIP).
class IProductionQueueRepository {
public:
    virtual ~IProductionQueueRepository() = default;

    virtual std::optional<ProductionQueueEntry> findById(const std::string& orderId) const = 0;
    virtual std::vector<ProductionQueueEntry> findAll() const = 0; // 0번째 = 큐 head(FIFO)
    virtual void save(const ProductionQueueEntry& entry) = 0;      // 신규면 큐 끝에 추가, 기존이면 갱신
    virtual void remove(const std::string& orderId) = 0;           // 생산 완료 후 큐에서 제거
};
