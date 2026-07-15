#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../Model/Sample.h"

// Service는 이 인터페이스에만 의존한다(design.md §4.2 Repository 패턴 + DIP).
class ISampleRepository {
public:
    virtual ~ISampleRepository() = default;

    virtual std::optional<Sample> findById(const std::string& id) const = 0;
    virtual std::vector<Sample> findAll() const = 0;
    virtual void save(const Sample& sample) = 0; // 변경 시 즉시 영속화
};
