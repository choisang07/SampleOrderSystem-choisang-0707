#pragma once

#include <stdexcept>
#include <string>

#include "../vendor/nlohmann/json.hpp"

// 주문 상태. requirement.md 4절 참고.
// REJECTED는 정상 흐름 밖의 상태로, 모니터링(상태별 목록/집계)에서는 제외한다(5.5절).
enum class OrderStatus {
    RESERVED,
    PRODUCING,
    CONFIRMED,
    RELEASE,
    REJECTED
};

inline std::string toString(OrderStatus status) {
    switch (status) {
        case OrderStatus::RESERVED:  return "RESERVED";
        case OrderStatus::PRODUCING: return "PRODUCING";
        case OrderStatus::CONFIRMED: return "CONFIRMED";
        case OrderStatus::RELEASE:   return "RELEASE";
        case OrderStatus::REJECTED:  return "REJECTED";
    }
    return "UNKNOWN";
}

inline OrderStatus orderStatusFromString(const std::string& text) {
    if (text == "RESERVED")  return OrderStatus::RESERVED;
    if (text == "PRODUCING") return OrderStatus::PRODUCING;
    if (text == "CONFIRMED") return OrderStatus::CONFIRMED;
    if (text == "RELEASE")   return OrderStatus::RELEASE;
    if (text == "REJECTED")  return OrderStatus::REJECTED;
    throw std::invalid_argument("Unknown OrderStatus: " + text);
}

// 시료 주문(Order). requirement.md 5.3, 5.4 참고.
// 상태 전이 로직(State 패턴)은 Phase 2~3에서 Domain/OrderState에 추가된다.
// 여기서는 순수 데이터 구조 + JSON 매핑만 담당한다(design.md §3, §5).
struct Order {
    std::string id;
    std::string sampleId;
    std::string customerName;
    int quantity = 0;
    OrderStatus status = OrderStatus::RESERVED;
    std::string createdAt; // "YYYY-MM-DD HH:MM:SS" (표시/정렬용)
};

inline void to_json(nlohmann::json& j, const Order& o) {
    j = nlohmann::json{
        {"id", o.id},
        {"sampleId", o.sampleId},
        {"customerName", o.customerName},
        {"quantity", o.quantity},
        {"status", toString(o.status)},
        {"createdAt", o.createdAt}
    };
}

inline void from_json(const nlohmann::json& j, Order& o) {
    j.at("id").get_to(o.id);
    j.at("sampleId").get_to(o.sampleId);
    j.at("customerName").get_to(o.customerName);
    j.at("quantity").get_to(o.quantity);
    o.status = orderStatusFromString(j.at("status").get<std::string>());
    j.at("createdAt").get_to(o.createdAt);
}
