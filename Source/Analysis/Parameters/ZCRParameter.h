#pragma once

#include "IAudioParameter.h"

class ZCRParameter : public IAudioParameter {
public:
    std::string getName() const override { return "zeroCrossingRate"; }

    ParameterValue compute(const float* samples, size_t count) const override {
        if (count < 2) return 0.0;

        size_t crossings = 0;
        for (size_t i = 1; i < count; ++i) {
            if ((samples[i] >= 0.0f) != (samples[i - 1] >= 0.0f))
                ++crossings;
        }
        return static_cast<double>(crossings) / static_cast<double>(count - 1);
    }
};
