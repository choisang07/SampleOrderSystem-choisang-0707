#pragma once

#include <optional>
#include <string>

#include "../Model/ProductionQueueEntry.h"
#include "../Repository/IOrderRepository.h"
#include "../Repository/IProductionQueueRepository.h"
#include "../Repository/ISampleRepository.h"

// 생산 큐 오케스트레이션(design.md §4.3). 단일 글로벌 FIFO 큐(§10 결정)이며,
// 큐 상태는 자체 멤버로 들고 있지 않고 IProductionQueueRepository를 통해서만
// 읽고 쓴다(§4.2 Repository+DIP). 완료 처리 시 재고 반영(ISampleRepository)과
// 주문 상태 전이(IOrderRepository)까지 함께 책임진다(abnormal.md 3번 확정 —
// 3-인자 생성자).
class ProductionService {
public:
    ProductionService(IOrderRepository& orderRepo, IProductionQueueRepository& queueRepo,
                       ISampleRepository& sampleRepo);

    // 큐 끝에 추가한다. 큐가 비어 있었다면 startedAt을 현재 wall-clock 시각으로 채워
    // "즉시 생산 시작" 처리하고(design.md §6.2), 아니면 startedAt을 빈 문자열로 유지해
    // "대기 중"으로 등록한다.
    void enqueue(const ProductionQueueEntry& entry);

    // 큐 head(findAll()의 0번째)를 반환한다. 큐가 비어 있으면 std::nullopt.
    std::optional<ProductionQueueEntry> peekActive() const;

    // 큐 head의 startedAt + totalProductionMinutes가 현재 시각을 지났으면(lazy 체크,
    // design.md §10) 완료 처리한다:
    //   1) requiredQuantity 전량을 수율 재적용 없이 Sample.stock에 반영(규칙 4)
    //   2) 해당 주문을 PRODUCING -> CONFIRMED로 전이(orderRepo 저장)
    //   3) 큐에서 제거
    //   4) 다음 항목이 있으면 startedAt을 채워 생산 시작 처리
    // 완료 시점 미도달이면 아무 부수효과도 없다.
    void completeActive();

    // 큐 head의 현재 경과 시간(분). startedAt이 비어 있으면(아직 시작 전) 0을 반환한다.
    // 생산 라인 조회 화면의 진행률/완료 예정 계산에 사용한다(design.md §6.2, wall-clock 기준).
    double elapsedMinutesOf(const ProductionQueueEntry& entry) const;

    // startedAt + totalProductionMinutes 시점을 "HH:MM" 형태로 반환한다(완료 예정 시각).
    // startedAt이 비어 있으면 빈 문자열을 반환한다.
    std::string estimatedCompletionTimeOf(const ProductionQueueEntry& entry) const;

private:
    IOrderRepository& orderRepo_;
    IProductionQueueRepository& queueRepo_;
    ISampleRepository& sampleRepo_;
};
