#pragma once

#include <string>
#include <vector>

#include "../Model/Sample.h"

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

    // [1] 시료 관리 하위 메뉴(등록/목록/검색/뒤로가기) 출력(docs/screens.md 기준).
    void printSampleManagementMenu() const;

    // 임의의 프롬프트 문구를 출력하고 한 줄 입력을 받는다(promptCommand와 동일한
    // EOF 처리 계약 — 호출 후 isInputAlive()로 종료 여부를 확인해야 한다).
    std::string promptLine(const std::string& prompt) const;

    // 시료 목록(등록/조회/검색 결과 공용)을 표 형태로 출력한다.
    void printSampleList(const std::vector<Sample>& samples) const;

    // [2] 시료 주문 메뉴 헤더(docs/screens.md 기준).
    void printOrderMenuHeader() const;

    // 입력 내용 확인 화면(시료명/시료ID/고객명/수량) + [Y]/[N] 안내 출력.
    void printOrderConfirmation(const std::string& sampleName, const std::string& sampleId,
                                const std::string& customerName, int quantity) const;

    // 예약 접수 완료 화면(주문번호/현재 상태) 출력.
    void printOrderReservationComplete(const std::string& orderId, const std::string& status) const;
};
