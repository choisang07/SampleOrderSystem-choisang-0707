#include "MainController.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../Domain/OrderState/IOrderState.h"
#include "../Service/ProductionService.h"

#include "../Model/Order.h"
#include "../Service/OrderService.h"
#include "../Service/ReleaseService.h"

namespace {

// "YYYY-MM-DD HH:MM:SS" 형식의 현재 시각 문자열(design.md §5 createdAt 포맷과 동일).
std::string currentTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTm{};
#ifdef _WIN32
    localtime_s(&localTm, &nowTime);
#else
    localtime_r(&nowTime, &localTm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&localTm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

}  // namespace

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
            handleApprovalRejection();
        } else if (command == "4") {
            handleMonitoring();
        } else if (command == "5") {
            handleProductionLine();
        } else if (command == "6") {
            handleRelease();
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
    // [2] 시료 주문(예약 접수) — docs/screens.md "[2] 시료 주문" 흐름.
    // 재고 확인/차감은 절대 하지 않는다(Phase 3 OrderService::approve의 책임, CLAUDE.md 3절).
    view_.printOrderMenuHeader();

    const std::string sampleId = view_.promptLine("시료 ID > ");
    if (!view_.isInputAlive()) {
        return;
    }
    const std::string customerName = view_.promptLine("고객명 > ");
    if (!view_.isInputAlive()) {
        return;
    }
    const std::string quantityInput = view_.promptLine("주문 수량 > ");
    if (!view_.isInputAlive()) {
        return;
    }

    int quantity = 0;
    try {
        size_t pos = 0;
        quantity = std::stoi(quantityInput, &pos);
        if (pos != quantityInput.size()) {
            throw std::invalid_argument("trailing characters");
        }
    } catch (const std::exception&) {
        view_.printMessage("숫자를 입력하세요.");
        return;
    }

    SampleService sampleService(sampleRepo_);
    const auto sampleOpt = sampleService.findById(sampleId);
    if (!sampleOpt.has_value()) {
        view_.printMessage("등록되지 않은 시료입니다.");
        return;
    }

    view_.printOrderConfirmation(sampleOpt->name, sampleOpt->id, customerName, quantity);
    const std::string confirm = view_.promptLine("선택 > ");
    if (!view_.isInputAlive()) {
        return;
    }

    if (confirm != "Y" && confirm != "y") {
        view_.printMessage("예약이 취소되었습니다.");
        return;
    }

    OrderService service(orderRepo_, sampleRepo_);
    try {
        const Order order = service.reserve(sampleId, customerName, quantity);
        view_.printOrderReservationComplete(order.id, toString(order.status));
    } catch (const std::invalid_argument& e) {
        view_.printMessage(e.what());
    }
}

void MainController::handleMonitoring() {
    // TODO(Phase 5): MonitorService를 통한 상태별 집계/재고 현황 메뉴 구현.
    view_.printMessage("[모니터링] 기능은 Phase 5에서 구현됩니다.");
}

void MainController::handleRelease() {
    ReleaseService service(orderRepo_);

    view_.printReleaseMenu();

    const auto releasable = service.listReleasable();
    if (releasable.empty()) {
        view_.printMessage("출고 가능한 주문이 없습니다.");
        return;
    }

    view_.printReleasableOrderList(releasable);
    const std::string input = view_.promptLine("출고할 번호 > ");
    if (!view_.isInputAlive()) {
        return;
    }

    int index = 0;
    try {
        size_t pos = 0;
        index = std::stoi(input, &pos);
        if (pos != input.size()) {
            throw std::invalid_argument("trailing characters");
        }
    } catch (const std::exception&) {
        view_.printMessage("잘못된 입력입니다.");
        return;
    }

    if (index < 1 || static_cast<size_t>(index) > releasable.size()) {
        view_.printMessage("잘못된 입력입니다.");
        return;
    }

    try {
        const Order released = service.release(releasable[static_cast<size_t>(index - 1)].id);
        view_.printReleaseComplete(released, currentTimestamp());
    } catch (const std::invalid_argument& e) {
        view_.printMessage(e.what());
    }
}

void MainController::handleApprovalRejection() {
    SampleService sampleService(sampleRepo_);
    ProductionService productionService(orderRepo_, productionQueueRepo_, sampleRepo_);

    // 메뉴 진입 시점에 생산완료 lazy 체크(design.md §10) — 방금 완료된 생산분이
    // 재고에 반영된 뒤의 최신 상태를 기준으로 승인 판단이 이뤄지도록 한다.
    productionService.completeActive();

    bool inSubMenu = true;
    while (inSubMenu) {
        std::vector<Order> reserved;
        for (const auto& order : orderRepo_.findAll()) {
            if (order.status == OrderStatus::RESERVED) {
                reserved.push_back(order);
            }
        }

        view_.printApprovalRejectionMenu();
        if (reserved.empty()) {
            view_.printMessage("승인 대기 중인 예약 주문이 없습니다.");
            break;
        }

        std::vector<ReservedOrderView> rows;
        for (const auto& order : reserved) {
            ReservedOrderView row;
            row.orderId = order.id;
            row.customerName = order.customerName;
            const auto sampleOpt = sampleService.findById(order.sampleId);
            row.sampleName = sampleOpt ? sampleOpt->name : order.sampleId;
            row.quantity = order.quantity;
            rows.push_back(row);
        }
        view_.printReservedOrderList(rows);

        const std::string command = view_.promptCommand();
        if (!view_.isInputAlive()) {
            inSubMenu = false;
            break;
        }
        if (command == "0") {
            inSubMenu = false;
            continue;
        }

        int index = 0;
        try {
            size_t pos = 0;
            index = std::stoi(command, &pos);
            if (pos != command.size()) {
                throw std::invalid_argument("trailing characters");
            }
        } catch (const std::exception&) {
            view_.printMessage("잘못된 입력입니다.");
            continue;
        }
        if (index < 1 || static_cast<size_t>(index) > reserved.size()) {
            view_.printMessage("잘못된 입력입니다.");
            continue;
        }

        Order order = reserved[static_cast<size_t>(index) - 1];
        const auto sampleOpt = sampleService.findById(order.sampleId);
        if (!sampleOpt) {
            view_.printMessage("등록되지 않은 시료입니다.");
            continue;
        }
        const Sample sample = *sampleOpt;
        const int covered = std::min(sample.stock, order.quantity);
        const int shortfall = order.quantity - covered;
        int requiredQuantity = 0;
        double totalMinutes = 0.0;
        if (shortfall > 0) {
            requiredQuantity = static_cast<int>(std::ceil(static_cast<double>(shortfall) / sample.yield));
            totalMinutes = sample.avgProductionTime * requiredQuantity;
        }
        view_.printApprovalPreview(sample.name, sample.stock, order.quantity, shortfall, requiredQuantity,
                                    totalMinutes);

        const std::string decision = view_.promptCommand();
        if (!view_.isInputAlive()) {
            inSubMenu = false;
            break;
        }

        try {
            if (decision == "Y" || decision == "y") {
                const std::string fromStatus = toString(order.status);
                stateFor(order.status).approve(order, sampleService, productionService);
                orderRepo_.save(order);
                view_.printApprovalResult(order.id, fromStatus, toString(order.status));
            } else if (decision == "N" || decision == "n") {
                stateFor(order.status).reject(order);
                orderRepo_.save(order);
                view_.printRejectionResult(order.id);
            } else {
                view_.printMessage("잘못된 입력입니다.");
            }
        } catch (const std::logic_error& e) {
            // std::invalid_argument는 std::logic_error의 서브클래스이므로 이 한 catch로
            // "잘못된 상태 전이"(abnormal.md 1번 확정)와 "등록되지 않은 시료" 모두 처리된다.
            view_.printMessage(e.what());
        }
    }
}

void MainController::handleProductionLine() {
    ProductionService productionService(orderRepo_, productionQueueRepo_, sampleRepo_);

    // 메뉴 진입 시점에 생산완료 lazy 체크(design.md §10).
    productionService.completeActive();

    view_.printProductionLineMenu();

    const auto queue = productionQueueRepo_.findAll();
    if (queue.empty()) {
        view_.printMessage("생산 대기/진행 중인 항목이 없습니다.");
    } else {
        const ProductionQueueEntry& head = queue.front();
        const auto sampleOpt = sampleRepo_.findById(head.sampleId);

        ProductionLineActiveView activeView;
        activeView.orderId = head.orderId;
        activeView.sampleName = sampleOpt ? sampleOpt->name : head.sampleId;
        activeView.requiredQuantity = head.requiredQuantity;
        activeView.totalProductionMinutes = head.totalProductionMinutes;
        activeView.elapsedMinutes = productionService.elapsedMinutesOf(head);
        activeView.estimatedCompletion = productionService.estimatedCompletionTimeOf(head);
        view_.printProductionLineActive(activeView);

        std::vector<ProductionLineWaitingView> waitingRows;
        for (size_t i = 1; i < queue.size(); ++i) {
            const auto& entry = queue[i];
            const auto waitingSampleOpt = sampleRepo_.findById(entry.sampleId);
            ProductionLineWaitingView row;
            row.order = static_cast<int>(i);
            row.orderId = entry.orderId;
            row.sampleName = waitingSampleOpt ? waitingSampleOpt->name : entry.sampleId;
            row.requiredQuantity = entry.requiredQuantity;
            waitingRows.push_back(row);
        }
        view_.printProductionLineWaiting(waitingRows);
    }

    view_.promptLine("선택 > ");
}
