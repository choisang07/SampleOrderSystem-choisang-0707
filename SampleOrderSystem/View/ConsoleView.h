#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../Model/Order.h"
#include "../Model/ProductionQueueEntry.h"
#include "../Model/Sample.h"

// 생산 라인 조회 화면 전용 표시용 DTO(View 전담, design.md §4.3의 ProductionQueueEntry를
// 화면에 필요한 부가 정보(시료명, 진행률/완료예정)와 함께 보여주기 위한 것으로 로직은 없다.
struct ProductionLineActiveView {
    std::string orderId;
    std::string sampleName;
    int requiredQuantity = 0;
    double totalProductionMinutes = 0.0;
    double elapsedMinutes = 0.0;
    std::string estimatedCompletion;  // "HH:MM"
};

struct ProductionLineWaitingView {
    int order = 0;  // 대기 순번(1부터)
    std::string orderId;
    std::string sampleName;
    int requiredQuantity = 0;
};

// 승인/거절 화면에서 사용하는 예약 목록 행(고객/시료명까지 포함해 표시).
struct ReservedOrderView {
    std::string orderId;
    std::string customerName;
    std::string sampleName;
    int quantity = 0;
};

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

    // [6] 출고 처리 메뉴 출력(docs/screens.md 기준).
    void printReleaseMenu() const;

    // 출고 가능(CONFIRMED) 주문 목록을 번호와 함께 출력한다.
    void printReleasableOrderList(const std::vector<Order>& orders) const;

    // 출고 처리 완료 안내(주문번호/출고수량/처리일시/상태)를 출력한다.
    void printReleaseComplete(const Order& order, const std::string& processedAt) const;

    // [3] 주문 승인/거절 화면(docs/screens.md 기준).
    void printApprovalRejectionMenu() const;
    void printReservedOrderList(const std::vector<ReservedOrderView>& orders) const;
    void printApprovalPreview(const std::string& sampleName, int currentStock, int orderQuantity,
                               int shortfall, int requiredQuantity, double totalProductionMinutes) const;
    void printApprovalResult(const std::string& orderId, const std::string& fromStatus,
                              const std::string& toStatus) const;
    void printRejectionResult(const std::string& orderId) const;

    // [5] 생산라인 조회 화면(docs/screens.md 기준).
    void printProductionLineMenu() const;
    void printProductionLineActive(const ProductionLineActiveView& active) const;
    void printProductionLineWaiting(const std::vector<ProductionLineWaitingView>& waiting) const;
};
