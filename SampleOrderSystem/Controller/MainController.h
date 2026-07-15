#pragma once

#include "../Repository/IOrderRepository.h"
#include "../Repository/IProductionQueueRepository.h"
#include "../Repository/ISampleRepository.h"
#include "../Service/SampleService.h"
#include "../View/ConsoleView.h"

// 메인 메뉴 라우팅 담당(design.md §3, §7). Phase 0에서는 메뉴 뼈대만 있고,
// 실제 시료/주문/모니터링/출고/생산 기능은 Phase 1~5에서 각 Service 계층과
// 연동하며 채워진다(Controller는 Service를 통해서만 데이터를 조작한다는
// ConsoleMVC PoC 원칙을 계승한다).
// Controller는 Persistence 구체 클래스(JsonFileStore)를 알지 못한다(DIP, design.md §3, §8).
// 데이터 파일 로드 성공 여부는 main.cpp가 확인해 bool 하나로만 전달받는다.
class MainController {
public:
    MainController(bool dataLoadFailed,
                    ISampleRepository& sampleRepo,
                    IOrderRepository& orderRepo,
                    IProductionQueueRepository& productionQueueRepo,
                    ConsoleView& view);

    void run();

private:
    void handleSampleManagement();
    void handleSampleRegistration(SampleService& service);
    void handleSampleList(SampleService& service);
    void handleSampleSearch(SampleService& service);
    void handleOrder();
    void handleMonitoring();
    void handleRelease();
    void handleProductionLine();

    bool dataLoadFailed_;
    ISampleRepository& sampleRepo_;
    IOrderRepository& orderRepo_;
    IProductionQueueRepository& productionQueueRepo_;
    ConsoleView& view_;
};
