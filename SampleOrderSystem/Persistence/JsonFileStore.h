#pragma once

#include <string>
#include <vector>

#include "../Model/Order.h"
#include "../Model/ProductionQueueEntry.h"
#include "../Model/Sample.h"

// store.json 한 파일에 samples/orders/productionQueue 3개 배열을 담아
// load/save를 전담하는 순수 파일 I/O 계층 (design.md §4.2, §5).
// 각 Repository는 자신이 다루는 컬렉션만 읽고 쓰지만, 파일 자체는 하나이므로
// save 시에는 다른 컬렉션의 기존 값을 그대로 보존한 채 자신의 컬렉션만 갱신한다.
class JsonFileStore {
public:
    explicit JsonFileStore(std::string filePath);

    // 파일이 없거나 손상되었으면 빈 목록을 반환한다(§8 이슈 반영: 실패는 Controller가
    // load() 시점에만 배너로 알리므로, 이 개별 load*는 조용히 빈 값으로 시작한다).
    std::vector<Sample> loadSamples() const;
    void saveSamples(const std::vector<Sample>& samples);

    std::vector<Order> loadOrders() const;
    void saveOrders(const std::vector<Order>& orders);

    std::vector<ProductionQueueEntry> loadProductionQueue() const;
    void saveProductionQueue(const std::vector<ProductionQueueEntry>& queue);

    // 파일이 존재하는데 JSON 파싱에 실패한 경우(손상)만 true를 반환한다.
    // 파일이 아예 없는 경우(최초 실행)는 정상 흐름이므로 false를 반환한다.
    bool isCorrupted() const;

private:
    // 파일을 열고 파싱을 시도한다. fileExists는 파일이 존재해 열렸는지,
    // parsedOk는 파싱에 성공했는지를 각각 담는다(파일이 없으면 fileExists=false,
    // parsedOk=false). loadRoot()/isCorrupted()가 이 헬퍼 하나만 공유하므로
    // 파일을 두 번 여는 일이 없다.
    nlohmann::json tryLoadRoot(bool& fileExists, bool& parsedOk) const;
    nlohmann::json loadRoot() const;
    void saveRoot(const nlohmann::json& root) const;

    std::string filePath_;
};
