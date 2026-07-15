#pragma once

#include "../Persistence/JsonFileStore.h"
#include "../Repository/IOrderRepository.h"
#include "../Repository/IProductionQueueRepository.h"
#include "../Repository/ISampleRepository.h"
#include "../View/ConsoleView.h"

// 메인 메뉴 라우팅 담당(design.md §3, §7). Phase 0에서는 메뉴 뼈대만 있고,
// 실제 시료/주문/모니터링/출고/생산 기능은 Phase 1~5에서 각 Service 계층과
// 연동하며 채워진다(Controller는 Service를 통해서만 데이터를 조작한다는
// ConsoleMVC PoC 원칙을 계승한다).
class MainController {
public:
    MainController(JsonFileStore& store,
                    ISampleRepository& sampleRepo,
                    IOrderRepository& orderRepo,
                    IProductionQueueRepository& productionQueueRepo,
                    ConsoleView& view);

    void run();

private:
    void handleSampleManagement();
    void handleOrder();
    void handleMonitoring();
    void handleRelease();
    void handleProductionLine();

    JsonFileStore& store_;
    ISampleRepository& sampleRepo_;
    IOrderRepository& orderRepo_;
    IProductionQueueRepository& productionQueueRepo_;
    ConsoleView& view_;
};
