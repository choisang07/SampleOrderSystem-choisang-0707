#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../Model/Sample.h"
#include "../Repository/ISampleRepository.h"

// 시료 등록/조회/검색 오케스트레이션(design.md §3). Controller는 이 Service를
// 통해서만 시료 데이터를 다룬다.
class SampleService {
public:
    explicit SampleService(ISampleRepository& repo);

    // SampleFactory로 검증/채번을 위임한 뒤 저장한다. 유효성 위반 시
    // std::invalid_argument가 그대로 전파되며 save는 호출되지 않는다.
    Sample registerSample(const std::string& name, double avgProductionTime, double yield);

    // 등록된 모든 시료 목록(재고 포함)을 반환한다.
    std::vector<Sample> listAll() const;

    // ID로 단건 조회. 존재하지 않으면 std::nullopt를 반환한다.
    // Controller가 Repository를 직접 참조하지 않도록 하는 창구(design.md §3).
    std::optional<Sample> findById(const std::string& id) const;

    // 이름(name) 대상 부분 일치(contains) + 대소문자 무시 검색.
    // keyword가 빈 문자열이면 std::invalid_argument를 던진다(재입력 요구, 확정 사항).
    std::vector<Sample> search(const std::string& keyword) const;

private:
    ISampleRepository& repo_;
};
