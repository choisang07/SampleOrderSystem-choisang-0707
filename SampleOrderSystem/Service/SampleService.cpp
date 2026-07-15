#include "SampleService.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

#include "../Domain/Factory/SampleFactory.h"

namespace {

std::string toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

}  // namespace

SampleService::SampleService(ISampleRepository& repo) : repo_(repo) {
}

Sample SampleService::registerSample(const std::string& name, double avgProductionTime, double yield) {
    Sample sample = SampleFactory::create(name, avgProductionTime, yield, repo_);
    repo_.save(sample);
    return sample;
}

std::vector<Sample> SampleService::listAll() const {
    return repo_.findAll();
}

std::vector<Sample> SampleService::search(const std::string& keyword) const {
    if (keyword.empty() || SampleFactory::isBlank(keyword)) {
        throw std::invalid_argument("검색어를 입력하세요.");
    }

    const std::string loweredKeyword = toLower(keyword);
    std::vector<Sample> result;
    for (const auto& sample : repo_.findAll()) {
        if (toLower(sample.name).find(loweredKeyword) != std::string::npos) {
            result.push_back(sample);
        }
    }
    return result;
}
