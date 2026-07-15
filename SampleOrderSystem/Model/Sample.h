#pragma once

#include <string>

#include "../vendor/nlohmann/json.hpp"

// 시료(Sample). requirement.md 5.2절 참고.
struct Sample {
    std::string id;
    std::string name;
    double avgProductionTime = 0.0; // 평균 생산시간 (min/ea)
    double yield = 1.0;             // 수율 (0~1)
    int stock = 0;                  // 현재 재고 수량
};

inline void to_json(nlohmann::json& j, const Sample& s) {
    j = nlohmann::json{
        {"id", s.id},
        {"name", s.name},
        {"avgProductionTime", s.avgProductionTime},
        {"yield", s.yield},
        {"stock", s.stock}
    };
}

inline void from_json(const nlohmann::json& j, Sample& s) {
    j.at("id").get_to(s.id);
    j.at("name").get_to(s.name);
    j.at("avgProductionTime").get_to(s.avgProductionTime);
    j.at("yield").get_to(s.yield);
    j.at("stock").get_to(s.stock);
}
