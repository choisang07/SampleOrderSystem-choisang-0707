#include "ProductionService.h"

#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "../Domain/OrderState/IOrderState.h"
#include "SampleService.h"

namespace {

// "YYYY-MM-DD HH:MM:SS" 포맷(design.md §5)으로 현재 wall-clock 시각을 만든다.
std::string nowString() {
    const std::time_t t = std::time(nullptr);
    std::tm tmBuf{};
#ifdef _WIN32
    localtime_s(&tmBuf, &t);
#else
    localtime_r(&t, &tmBuf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// "YYYY-MM-DD HH:MM:SS" 문자열을 std::time_t로 변환한다.
std::time_t parseTime(const std::string& text) {
    std::tm tmBuf{};
    std::istringstream iss(text);
    iss >> std::get_time(&tmBuf, "%Y-%m-%d %H:%M:%S");
    if (iss.fail()) {
        throw std::invalid_argument("잘못된 시각 형식입니다: " + text);
    }
    tmBuf.tm_isdst = -1;
    return std::mktime(&tmBuf);
}

}  // namespace

ProductionService::ProductionService(IOrderRepository& orderRepo, IProductionQueueRepository& queueRepo,
                                      ISampleRepository& sampleRepo)
    : orderRepo_(orderRepo), queueRepo_(queueRepo), sampleRepo_(sampleRepo) {
}

void ProductionService::enqueue(const ProductionQueueEntry& entry) {
    ProductionQueueEntry toSave = entry;
    const bool wasEmpty = queueRepo_.findAll().empty();
    if (wasEmpty) {
        toSave.startedAt = nowString();  // 즉시 생산 시작(design.md §6.2)
    } else {
        toSave.startedAt.clear();  // 대기 중
    }
    queueRepo_.save(toSave);
}

std::optional<ProductionQueueEntry> ProductionService::peekActive() const {
    const auto all = queueRepo_.findAll();
    if (all.empty()) {
        return std::nullopt;
    }
    return all.front();
}

void ProductionService::completeActive() {
    const auto all = queueRepo_.findAll();
    if (all.empty()) {
        return;
    }

    const ProductionQueueEntry head = all.front();
    if (head.startedAt.empty()) {
        return;  // 아직 시작되지 않은 항목(정상적으로는 head는 항상 시작되어 있어야 함)
    }

    if (elapsedMinutesOf(head) < head.totalProductionMinutes) {
        return;  // 완료 시점 미도달(lazy 체크, design.md §10)
    }

    // 1) 재고 반영 — requiredQuantity 전량, 수율 재적용 없음(규칙 4).
    const auto sampleOpt = sampleRepo_.findById(head.sampleId);
    if (sampleOpt) {
        Sample sample = *sampleOpt;
        sample.stock += head.requiredQuantity;
        sampleRepo_.save(sample);
    }

    // 2) 주문 상태 전이(PRODUCING -> CONFIRMED).
    const auto orderOpt = orderRepo_.findById(head.orderId);
    if (orderOpt) {
        Order order = *orderOpt;
        SampleService sampleService(sampleRepo_);
        stateFor(order.status).completeProduction(order, sampleService);
        orderRepo_.save(order);
    }

    // 3) 큐에서 제거.
    queueRepo_.remove(head.orderId);

    // 4) 다음 항목 생산 시작 처리.
    const auto remaining = queueRepo_.findAll();
    if (!remaining.empty() && remaining.front().startedAt.empty()) {
        ProductionQueueEntry next = remaining.front();
        next.startedAt = nowString();
        queueRepo_.save(next);
    }
}

double ProductionService::elapsedMinutesOf(const ProductionQueueEntry& entry) const {
    if (entry.startedAt.empty()) {
        return 0.0;
    }
    const std::time_t started = parseTime(entry.startedAt);
    const std::time_t now = std::time(nullptr);
    return std::difftime(now, started) / 60.0;
}

std::string ProductionService::estimatedCompletionTimeOf(const ProductionQueueEntry& entry) const {
    if (entry.startedAt.empty()) {
        return "";
    }
    std::time_t started = parseTime(entry.startedAt);
    started += static_cast<std::time_t>(entry.totalProductionMinutes * 60.0);

    std::tm tmBuf{};
#ifdef _WIN32
    localtime_s(&tmBuf, &started);
#else
    localtime_r(&started, &tmBuf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%H:%M");
    return oss.str();
}
