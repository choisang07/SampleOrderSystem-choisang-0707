#include "SampleFactory.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {

bool isBlank(const std::string& s) {
    return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); });
}

}  // namespace

Sample SampleFactory::create(const std::string& name, double avgProductionTime, double yield,
                              const ISampleRepository& repo) {
    if (name.empty() || isBlank(name)) {
        throw std::invalid_argument("이름은 필수입니다.");
    }
    if (avgProductionTime <= 0.0) {
        throw std::invalid_argument("평균 생산시간은 0보다 커야 합니다.");
    }
    if (yield <= 0.0 || yield > 1.0) {
        throw std::invalid_argument("수율은 0보다 크고 1 이하여야 합니다.");
    }

    Sample sample;
    sample.id = nextId(repo);
    sample.name = name;
    sample.avgProductionTime = avgProductionTime;
    sample.yield = yield;
    sample.stock = 0;  // 확정: 등록 메뉴는 재고를 입력받지 않는다.
    return sample;
}

std::string SampleFactory::nextId(const ISampleRepository& repo) {
    const std::string prefix = "S-";
    int maxSeq = 0;
    for (const auto& sample : repo.findAll()) {
        if (sample.id.rfind(prefix, 0) != 0) {
            continue;  // "S-" 접두사가 아니면 채번 대상에서 제외.
        }
        const std::string suffix = sample.id.substr(prefix.size());
        if (suffix.empty() || !std::all_of(suffix.begin(), suffix.end(), ::isdigit)) {
            continue;
        }
        const int seq = std::stoi(suffix);
        maxSeq = std::max(maxSeq, seq);
    }

    std::ostringstream oss;
    oss << prefix << std::setw(3) << std::setfill('0') << (maxSeq + 1);
    return oss.str();
}
