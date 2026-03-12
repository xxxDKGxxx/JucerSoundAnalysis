#pragma once

#include <cstddef>
#include <string>
#include <variant>
#include <vector>

using ParameterValue = std::variant<double, bool, std::vector<double>>;

class IAudioParameter {
public:
    virtual ~IAudioParameter() = default;
    virtual std::string getName() const = 0;
    virtual ParameterValue compute(const float* samples, size_t count) const = 0;
};
