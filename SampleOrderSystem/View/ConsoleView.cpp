#include "ConsoleView.h"

#include <iomanip>
#include <iostream>

void ConsoleView::printMainMenu() const {
    // docs/screens.md "메인 메뉴" 확정 6개 항목(design.md §7)에 맞춘 메뉴 구성.
    std::cout << "==============================\n";
    std::cout << " 반도체 시료 생산주문관리 시스템\n";
    std::cout << "==============================\n";
    std::cout << "[1] 시료 관리       [2] 시료 주문\n";
    std::cout << "[3] 주문 승인/거절  [4] 모니터링\n";
    std::cout << "[5] 생산라인 조회   [6] 출고 처리\n";
    std::cout << "[0] 종료\n";
    std::cout << "선택 > ";
}

void ConsoleView::printMessage(const std::string& message) const {
    std::cout << message << "\n";
}

void ConsoleView::printDataLoadFailureBanner() const {
    std::cout << "==============================\n";
    std::cout << " [경고] 데이터 파일 로드 실패\n";
    std::cout << " store.json 파일이 손상되었거나 형식이 올바르지 않습니다.\n";
    std::cout << " 빈 데이터로 시작합니다.\n";
    std::cout << "==============================\n";
}

std::string ConsoleView::promptCommand() const {
    std::string input;
    std::getline(std::cin, input);
    return input;
}

bool ConsoleView::isInputAlive() const {
    return static_cast<bool>(std::cin);
}

void ConsoleView::printSampleManagementMenu() const {
    std::cout << "__________________________________________________\n";
    std::cout << "[1] 시료 관리\n";
    std::cout << "__________________________________________________\n";
    std::cout << "[1] 시료 등록    [2] 시료 목록    [3] 시료 검색    [0] 뒤로\n";
    std::cout << "선택 > ";
}

std::string ConsoleView::promptLine(const std::string& prompt) const {
    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    return input;
}

void ConsoleView::printSampleList(const std::vector<Sample>& samples) const {
    std::cout << "등록 시료 목록 (총 " << samples.size() << " 종)\n";
    std::cout << "ID\t시료명\t평균 생산시간\t수율\t현재 재고\n";
    for (const auto& sample : samples) {
        std::cout << sample.id << "\t" << sample.name << "\t"
                   << sample.avgProductionTime << "\t" << sample.yield << "\t"
                   << sample.stock << "\n";
    }
}

void ConsoleView::printOrderMenuHeader() const {
    std::cout << "__________________________________________________\n";
    std::cout << "[2] 시료 주문\n";
    std::cout << "__________________________________________________\n";
}

void ConsoleView::printOrderConfirmation(const std::string& sampleName, const std::string& sampleId,
                                          const std::string& customerName, int quantity) const {
    std::cout << "\n입력 내용 확인\n\n";
    std::cout << "시료 " << sampleName << "    (" << sampleId << ")\n";
    std::cout << "고객    " << customerName << "\n";
    std::cout << "수량    " << quantity << " ea\n\n";
    std::cout << "[Y] 예약 접수    [N] 취소\n";
}

void ConsoleView::printOrderReservationComplete(const std::string& orderId, const std::string& status) const {
    std::cout << "\n예약 접수 완료.\n\n";
    std::cout << "주문번호    " << orderId << "\n";
    std::cout << "현재 상태 " << status << "\n";
}

void ConsoleView::printReleaseMenu() const {
    std::cout << "__________________________________________________\n";
    std::cout << "[6] 출고 처리\n";
    std::cout << "__________________________________________________\n";
}

void ConsoleView::printReleasableOrderList(const std::vector<Order>& orders) const {
    std::cout << "출고 가능 주문 (CONFIRMED)\n\n";
    std::cout << "번호\t주문번호\t고객\t시료\n";
    for (size_t i = 0; i < orders.size(); ++i) {
        std::cout << "[" << (i + 1) << "]\t" << orders[i].id << "\t"
                   << orders[i].customerName << "\t" << orders[i].sampleId << "\n";
    }
}

void ConsoleView::printReleaseComplete(const Order& order, const std::string& processedAt) const {
    std::cout << "\n출고 처리 완료\n\n";
    std::cout << "주문번호\t" << order.id << "\n";
    std::cout << "출고수량\t" << order.quantity << " ea\n";
    std::cout << "처리일시 " << processedAt << "\n";
    std::cout << "상태        " << toString(order.status) << "\n";
}

void ConsoleView::printApprovalRejectionMenu() const {
    std::cout << "__________________________________________________\n";
    std::cout << "[3] 주문 승인/거절\n";
    std::cout << "__________________________________________________\n";
    std::cout << "승인 대기 중인 예약 목록 (RESERVED)\n";
}

void ConsoleView::printReservedOrderList(const std::vector<ReservedOrderView>& orders) const {
    std::cout << "번호\t주문번호\t고객\t시료\t수량\n";
    for (size_t i = 0; i < orders.size(); ++i) {
        const auto& order = orders[i];
        std::cout << "[" << (i + 1) << "] " << order.orderId << "\t" << order.customerName << "\t"
                   << order.sampleName << "\t" << order.quantity << " ea\n";
    }
    std::cout << "승인할 번호(0: 뒤로) > ";
}

void ConsoleView::printApprovalPreview(const std::string& sampleName, int currentStock, int orderQuantity,
                                        int shortfall, int requiredQuantity,
                                        double totalProductionMinutes) const {
    std::cout << "재고 확인 중 ..\n\n";
    std::cout << "시료 " << sampleName << "    현재 재고 " << currentStock << " ea\n";
    std::cout << "주문 수량    " << orderQuantity << " ea    부족분    " << shortfall << " ea\n\n";
    if (shortfall <= 0) {
        std::cout << "재고 충분. 즉시 승인 가능합니다.\n";
    } else {
        std::cout << "재고 부족. 부족분 " << shortfall << " ea 승인하시겠습니까? (실생산량 "
                   << requiredQuantity << " ea / " << totalProductionMinutes << " min)\n";
    }
    std::cout << "[Y] 승인 [N] 주문 거절\n선택 > ";
}

void ConsoleView::printApprovalResult(const std::string& orderId, const std::string& fromStatus,
                                       const std::string& toStatus) const {
    std::cout << "승인 완료.\n\n";
    std::cout << "상태 변경    " << fromStatus << " -> " << toStatus << "\n";
    std::cout << "주문번호    " << orderId << "\n";
}

void ConsoleView::printRejectionResult(const std::string& orderId) const {
    std::cout << "거절 완료.\n\n";
    std::cout << "상태 변경    RESERVED -> REJECTED\n";
    std::cout << "주문번호    " << orderId << "\n";
}

void ConsoleView::printProductionLineMenu() const {
    std::cout << "__________________________________________________\n";
    std::cout << "[5] 생산라인 조회\n";
    std::cout << "__________________________________________________\n";
    std::cout << "생산라인 1개 (단일 라인)    현재 상태 : RUNNING\n";
}

void ConsoleView::printProductionLineActive(const ProductionLineActiveView& active) const {
    const double progress = active.totalProductionMinutes > 0.0
                                 ? (active.elapsedMinutes / active.totalProductionMinutes) * 100.0
                                 : 100.0;
    const double clampedProgress = progress < 0.0 ? 0.0 : (progress > 100.0 ? 100.0 : progress);

    std::cout << "\n현재 처리 중\n\n";
    std::cout << "주문번호    " << active.orderId << "     시료     " << active.sampleName << "\n";
    std::cout << "실생산량    " << active.requiredQuantity << " ea (" << active.totalProductionMinutes
               << "min)\n";
    std::cout << "진행        " << static_cast<int>(clampedProgress) << "%    완료 예정("
               << active.estimatedCompletion << ")\n";
}

void ConsoleView::printProductionLineWaiting(const std::vector<ProductionLineWaitingView>& waiting) const {
    std::cout << "\n대기 중인 주문\n\n";
    if (waiting.empty()) {
        std::cout << "대기 중인 주문이 없습니다.\n";
        return;
    }
    std::cout << "순서\t주문번호\t시료\t실생산량\n";
    for (const auto& row : waiting) {
        std::cout << row.order << "\t" << row.orderId << "\t" << row.sampleName << "\t"
                   << row.requiredQuantity << " ea\n";
    }
}
