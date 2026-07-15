#include "ConsoleView.h"

#include <iomanip>
#include <iostream>

void ConsoleView::printMainMenu() const {
    std::cout << "==============================\n";
    std::cout << " 반도체 시료 생산주문관리 시스템\n";
    std::cout << "==============================\n";
    std::cout << "[1] 시료 관리\n";
    std::cout << "[2] 주문 (접수/승인/거절)\n";
    std::cout << "[3] 모니터링\n";
    std::cout << "[4] 출고 처리\n";
    std::cout << "[5] 생산 라인\n";
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
