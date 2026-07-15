#include "MainController.h"

MainController::MainController(JsonFileStore& store,
                                 ISampleRepository& sampleRepo,
                                 IOrderRepository& orderRepo,
                                 IProductionQueueRepository& productionQueueRepo,
                                 ConsoleView& view)
    : store_(store),
      sampleRepo_(sampleRepo),
      orderRepo_(orderRepo),
      productionQueueRepo_(productionQueueRepo),
      view_(view) {
}

void MainController::run() {
    if (store_.isCorrupted()) {
        // design.md §8 이슈 반영: 로드 실패를 무언으로 넘기지 않고 배너로 알린다.
        view_.printDataLoadFailureBanner();
    }

    bool running = true;
    while (running) {
        view_.printMainMenu();
        const std::string command = view_.promptCommand();

        if (!view_.isInputAlive()) {
            // 표준입력 EOF 시 무한 루프 방지(design.md §8 이슈 반영).
            break;
        }

        if (command == "1") {
            handleSampleManagement();
        } else if (command == "2") {
            handleOrder();
        } else if (command == "3") {
            handleMonitoring();
        } else if (command == "4") {
            handleRelease();
        } else if (command == "5") {
            handleProductionLine();
        } else if (command == "0") {
            running = false;
        } else {
            view_.printMessage("잘못된 입력입니다.");
        }
    }
}

void MainController::handleSampleManagement() {
    // TODO(Phase 1): SampleService를 통한 등록/조회/검색 메뉴 구현.
    view_.printMessage("[시료 관리] 기능은 Phase 1에서 구현됩니다.");
}

void MainController::handleOrder() {
    // TODO(Phase 2~3): OrderService/ProductionService를 통한 접수/승인/거절 메뉴 구현.
    view_.printMessage("[주문 (접수/승인/거절)] 기능은 Phase 2~3에서 구현됩니다.");
}

void MainController::handleMonitoring() {
    // TODO(Phase 5): MonitorService를 통한 상태별 집계/재고 현황 메뉴 구현.
    view_.printMessage("[모니터링] 기능은 Phase 5에서 구현됩니다.");
}

void MainController::handleRelease() {
    // TODO(Phase 4): ReleaseService를 통한 출고 처리 메뉴 구현.
    view_.printMessage("[출고 처리] 기능은 Phase 4에서 구현됩니다.");
}

void MainController::handleProductionLine() {
    // TODO(Phase 3): ProductionService/MonitorService를 통한 생산 라인 조회 메뉴 구현.
    view_.printMessage("[생산 라인] 기능은 Phase 3에서 구현됩니다.");
}
