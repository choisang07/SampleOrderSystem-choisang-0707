#include "ConsoleView.h"

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
