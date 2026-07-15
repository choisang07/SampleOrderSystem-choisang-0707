#pragma once

#include <string>

#include "../../Model/Sample.h"
#include "../../Repository/ISampleRepository.h"

// 시료 생성 검증 + ID 자동 채번을 전담한다(design.md §4.4 Factory 패턴).
// Service/Controller는 이 Factory를 통해서만 신규 Sample을 만든다.
class SampleFactory {
public:
    // 유효성 검증(이름 필수, 평균생산시간>0, 수율 (0,1]) 후 ID를 자동 채번(S-{3자리 순번})해
    // Sample을 생성한다. stock은 항상 0으로 초기화된다(docs_temp/Phase1/testcase.md 확정 사항).
    // 검증 실패 시 std::invalid_argument를 던진다.
    static Sample create(const std::string& name, double avgProductionTime, double yield,
                          const ISampleRepository& repo);

private:
    static std::string nextId(const ISampleRepository& repo);
};
