#pragma once

#include "IAudioParameter.h"
#include <cmath>

class AMDFParameter : public IAudioParameter {
public:
    std::string getName() const override { return "amdf"; }

    ParameterValue compute(const float* samples, size_t count) const override {
        std::vector<double> result(count, 0.0);
        if (count == 0) return result;

        for (size_t lag = 0; lag < count; ++lag) {
            const size_t limit = count - lag;
            if (limit == 0) {
                result[lag] = 0.0;
                continue;
            }
            double sum = 0.0;
            for (size_t n = 0; n < limit; ++n) {
                sum += std::fabs(static_cast<double>(samples[n])
                               - static_cast<double>(samples[n + lag]));
            }
            result[lag] = sum / static_cast<double>(limit);
        }
        return result;
    }
};
