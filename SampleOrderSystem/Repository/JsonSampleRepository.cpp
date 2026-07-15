#include "JsonSampleRepository.h"

#include <algorithm>

JsonSampleRepository::JsonSampleRepository(JsonFileStore& store)
    : store_(store), samples_(store_.loadSamples()) {
}

std::optional<Sample> JsonSampleRepository::findById(const std::string& id) const {
    const auto it = std::find_if(samples_.begin(), samples_.end(),
        [&](const Sample& s) { return s.id == id; });
    if (it == samples_.end()) {
        return std::nullopt;
    }
    return *it;
}

std::vector<Sample> JsonSampleRepository::findAll() const {
    return samples_;
}

void JsonSampleRepository::save(const Sample& sample) {
    const auto it = std::find_if(samples_.begin(), samples_.end(),
        [&](const Sample& s) { return s.id == sample.id; });
    if (it == samples_.end()) {
        samples_.push_back(sample);
    } else {
        *it = sample;
    }
    persist();
}

void JsonSampleRepository::persist() {
    store_.saveSamples(samples_);
}
