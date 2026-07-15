#pragma once

#include <string>

// 콘솔 출력/입력만 전담한다. 로직은 없다(ConsoleMVC PoC 원칙 유지, design.md §3).
class ConsoleView {
public:
    void printMainMenu() const;
    void printMessage(const std::string& message) const;
    void printDataLoadFailureBanner() const;

    // 명령 입력을 받는다. 표준입력이 EOF로 종료되면(!std::cin) 빈 문자열을 반환한다
    // (design.md §8 이슈 반영 — 호출자는 !isInputAlive()로 종료 여부를 판단한다).
    std::string promptCommand() const;

    // 직전 promptCommand() 호출 이후 입력 스트림이 여전히 살아있는지 여부.
    bool isInputAlive() const;
};
