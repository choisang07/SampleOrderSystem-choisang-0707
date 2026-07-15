#include "OrderFactory.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {

bool isBlank(const std::string& s) {
    return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); });
}

std::tm currentLocalTime() {
    const std::time_t now = std::time(nullptr);
    std::tm tmValue{};
#ifdef _WIN32
    localtime_s(&tmValue, &now);
#else
    localtime_r(&now, &tmValue);
#endif
    return tmValue;
}

// 주문번호(ID): ORD-{YYYYMMDD}-{HHMMSS} (design.md §5, screens.md 확정 포맷).
// 참고(docs_temp/Phase2/abnormal.md 3번): 초 단위까지만 사용하므로 동일 초 내 연속
// 예약 시 ID가 충돌할 수 있음을 알고 있으나, 확정 포맷을 유지한다(Develope 판단).
std::string formatOrderId(const std::tm& t) {
    std::ostringstream oss;
    oss << "ORD-"
        << std::put_time(&t, "%Y%m%d") << "-"
        << std::put_time(&t, "%H%M%S");
    return oss.str();
}

// createdAt: "YYYY-MM-DD HH:MM:SS" (Model/Order.h 필드 주석, design.md §5 JSON 스키마).
std::string formatCreatedAt(const std::tm& t) {
    std::ostringstream oss;
    oss << std::put_time(&t, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

}  // namespace

Order OrderFactory::create(const std::string& sampleId, const std::string& customerName, int quantity,
                            const ISampleRepository& sampleRepo) {
    if (sampleId.empty() || !sampleRepo.findById(sampleId).has_value()) {
        throw std::invalid_argument("등록되지 않은 시료입니다.");
    }
    if (customerName.empty() || isBlank(customerName)) {
        throw std::invalid_argument("고객명을 입력하세요.");
    }
    if (quantity <= 0) {
        throw std::invalid_argument("주문 수량은 1 이상이어야 합니다.");
    }

    const std::tm now = currentLocalTime();

    Order order;
    order.id = formatOrderId(now);
    order.sampleId = sampleId;
    order.customerName = customerName;
    order.quantity = quantity;
    order.status = OrderStatus::RESERVED;
    order.createdAt = formatCreatedAt(now);
    return order;
}
