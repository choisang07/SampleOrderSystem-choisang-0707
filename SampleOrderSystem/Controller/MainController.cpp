#include "MainController.h"

#include <stdexcept>
#include <string>

MainController::MainController(bool dataLoadFailed,
                                 ISampleRepository& sampleRepo,
                                 IOrderRepository& orderRepo,
                                 IProductionQueueRepository& productionQueueRepo,
                                 ConsoleView& view)
    : dataLoadFailed_(dataLoadFailed),
      sampleRepo_(sampleRepo),
      orderRepo_(orderRepo),
      productionQueueRepo_(productionQueueRepo),
      view_(view) {
}

void MainController::run() {
    if (dataLoadFailed_) {
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
    SampleService service(sampleRepo_);

    bool inSubMenu = true;
    while (inSubMenu) {
        view_.printSampleManagementMenu();
        const std::string command = view_.promptCommand();

        if (!view_.isInputAlive()) {
            // 표준입력 EOF 시 무한 루프 방지(design.md §8 이슈 반영, 최상위 메뉴와 동일 패턴).
            inSubMenu = false;
            break;
        }

        if (command == "1") {
            handleSampleRegistration(service);
        } else if (command == "2") {
            handleSampleList(service);
        } else if (command == "3") {
            handleSampleSearch(service);
        } else if (command == "0") {
            inSubMenu = false;
        } else {
            view_.printMessage("잘못된 입력입니다.");
        }
    }
}

void MainController::handleSampleRegistration(SampleService& service) {
    const std::string name = view_.promptLine("이름 > ");
    if (!view_.isInputAlive()) {
        return;
    }
    const std::string avgInput = view_.promptLine("평균 생산시간 > ");
    if (!view_.isInputAlive()) {
        return;
    }
    const std::string yieldInput = view_.promptLine("수율 > ");
    if (!view_.isInputAlive()) {
        return;
    }

    double avgProductionTime = 0.0;
    double yield = 0.0;
    try {
        size_t pos = 0;
        avgProductionTime = std::stod(avgInput, &pos);
        if (pos != avgInput.size()) {
            throw std::invalid_argument("trailing characters");
        }
        pos = 0;
        yield = std::stod(yieldInput, &pos);
        if (pos != yieldInput.size()) {
            throw std::invalid_argument("trailing characters");
        }
    } catch (const std::exception&) {
        view_.printMessage("숫자를 입력하세요.");
        return;
    }

    try {
        const Sample sample = service.registerSample(name, avgProductionTime, yield);
        view_.printMessage("등록이 완료되었습니다. (ID: " + sample.id + ")");
    } catch (const std::invalid_argument& e) {
        view_.printMessage(e.what());
    }
}

void MainController::handleSampleList(SampleService& service) {
    const auto samples = service.listAll();
    if (samples.empty()) {
        view_.printMessage("등록된 시료가 없습니다.");
        return;
    }
    view_.printSampleList(samples);
}

void MainController::handleSampleSearch(SampleService& service) {
    bool searching = true;
    while (searching) {
        const std::string keyword = view_.promptLine("검색어 > ");
        if (!view_.isInputAlive()) {
            return;
        }

        try {
            const auto results = service.search(keyword);
            if (results.empty()) {
                view_.printMessage("검색 결과가 없습니다.");
            } else {
                view_.printSampleList(results);
            }
            searching = false;
        } catch (const std::invalid_argument&) {
            view_.printMessage("검색어를 입력하세요.");
            // 재입력 요구(확정, TC-P1-023) — 루프 계속.
        }
    }
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
