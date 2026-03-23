#pragma once

#include "IAudioParameter.h"

class AutocorrelationParameter : public IAudioParameter {
public:
  std::string getName() const override { return "autocorrelation"; }

  ParameterValue compute(const float *samples, size_t count,
                         double sampleRate) const override {
    std::vector<double> result(count, 0.0);
    if (count == 0)
      return result;

    for (size_t lag = 0; lag < count; ++lag) {
      double sum = 0.0;
      const size_t limit = count - lag;
      for (size_t n = 0; n < limit; ++n) {
        sum += static_cast<double>(samples[n]) *
               static_cast<double>(samples[n + lag]);
      }
      result[lag] = sum / static_cast<double>(count);
    }
    return result;
  }
};
