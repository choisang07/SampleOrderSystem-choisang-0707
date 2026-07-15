#include "JsonFileStore.h"

#include <fstream>

#include "../vendor/nlohmann/json.hpp"

JsonFileStore::JsonFileStore(std::string filePath) : filePath_(std::move(filePath)) {
}

nlohmann::json JsonFileStore::loadRoot() const {
    std::ifstream in(filePath_);
    if (!in.is_open()) {
        // 파일이 아직 없으면(최초 실행) 빈 문서로 시작한다.
        return nlohmann::json::object();
    }

    nlohmann::json root;
    try {
        in >> root;
    } catch (const nlohmann::json::parse_error&) {
        // 손상된 파일은 무시하고 빈 문서로 시작한다. isCorrupted()로 이 상황을
        // 별도로 확인할 수 있다(§8 이슈 반영).
        return nlohmann::json::object();
    }
    return root;
}

void JsonFileStore::saveRoot(const nlohmann::json& root) const {
    std::ofstream out(filePath_, std::ios::trunc);
    out << root.dump(2);
}

bool JsonFileStore::isCorrupted() const {
    std::ifstream in(filePath_);
    if (!in.is_open()) {
        return false; // 파일이 없는 것은 손상이 아니다.
    }

    nlohmann::json root;
    try {
        in >> root;
    } catch (const nlohmann::json::parse_error&) {
        return true;
    }
    return false;
}

std::vector<Sample> JsonFileStore::loadSamples() const {
    const nlohmann::json root = loadRoot();

    std::vector<Sample> samples;
    if (root.contains("samples") && root["samples"].is_array()) {
        for (const auto& item : root["samples"]) {
            samples.push_back(item.get<Sample>());
        }
    }
    return samples;
}

void JsonFileStore::saveSamples(const std::vector<Sample>& samples) {
    nlohmann::json root = loadRoot();
    root["samples"] = samples;
    saveRoot(root);
}

std::vector<Order> JsonFileStore::loadOrders() const {
    const nlohmann::json root = loadRoot();

    std::vector<Order> orders;
    if (root.contains("orders") && root["orders"].is_array()) {
        for (const auto& item : root["orders"]) {
            orders.push_back(item.get<Order>());
        }
    }
    return orders;
}

void JsonFileStore::saveOrders(const std::vector<Order>& orders) {
    nlohmann::json root = loadRoot();
    root["orders"] = orders;
    saveRoot(root);
}

std::vector<ProductionQueueEntry> JsonFileStore::loadProductionQueue() const {
    const nlohmann::json root = loadRoot();

    std::vector<ProductionQueueEntry> queue;
    if (root.contains("productionQueue") && root["productionQueue"].is_array()) {
        for (const auto& item : root["productionQueue"]) {
            queue.push_back(item.get<ProductionQueueEntry>());
        }
    }
    return queue;
}

void JsonFileStore::saveProductionQueue(const std::vector<ProductionQueueEntry>& queue) {
    nlohmann::json root = loadRoot();
    root["productionQueue"] = queue;
    saveRoot(root);
}
