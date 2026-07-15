#pragma once

#include <vector>

#include "../Persistence/JsonFileStore.h"
#include "ISampleRepository.h"

// store.json 기반 구현체(design.md §3, §4.2). 메모리 상태를 유지하며 변경 시
// 즉시 JsonFileStore를 통해 파일에 반영한다. 구체 클래스를 아는 곳은 main.cpp뿐이다.
class JsonSampleRepository : public ISampleRepository {
public:
    explicit JsonSampleRepository(JsonFileStore& store);

    std::optional<Sample> findById(const std::string& id) const override;
    std::vector<Sample> findAll() const override;
    void save(const Sample& sample) override;

private:
    void persist();

    JsonFileStore& store_;
    std::vector<Sample> samples_;
};
