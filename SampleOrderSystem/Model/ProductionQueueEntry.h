#pragma once

#include <string>

#include "../vendor/nlohmann/json.hpp"

// 생산 큐 항목. requirement.md 5.6 참고.
// - requiredQuantity  = ceil(부족분 / 수율) : 실 생산량
// - totalProductionMinutes = 평균 생산시간 * requiredQuantity
// - startedAt: 큐의 맨 앞(FIFO head)이라 실제로 생산이 시작된 항목만 값이 채워진다.
//   나머지 대기 항목은 빈 문자열("")이며, 큐 순번(FIFO)이 곧 생산 대기 순서다.
// - 생산 소요 시간은 wall-clock 기준이며, startedAt이 기록되어 있으면 프로그램을
//   재시작해도 (now - startedAt)으로 경과 시간을 그대로 복원할 수 있다 (규칙 3).
struct ProductionQueueEntry {
    std::string orderId;
    std::string sampleId;
    int requiredQuantity = 0;
    double totalProductionMinutes = 0.0;
    std::string startedAt; // "YYYY-MM-DD HH:MM:SS" 또는 "" (대기 중)
};

inline void to_json(nlohmann::json& j, const ProductionQueueEntry& p) {
    j = nlohmann::json{
        {"orderId", p.orderId},
        {"sampleId", p.sampleId},
        {"requiredQuantity", p.requiredQuantity},
        {"totalProductionMinutes", p.totalProductionMinutes},
        {"startedAt", p.startedAt}
    };
}

inline void from_json(const nlohmann::json& j, ProductionQueueEntry& p) {
    j.at("orderId").get_to(p.orderId);
    j.at("sampleId").get_to(p.sampleId);
    j.at("requiredQuantity").get_to(p.requiredQuantity);
    j.at("totalProductionMinutes").get_to(p.totalProductionMinutes);
    j.at("startedAt").get_to(p.startedAt);
}
