#pragma once

#include "IAudioParameter.h"
#include <cmath>

class VolumeParameter : public IAudioParameter {
public:
  std::string getName() const override { return "volume"; }

  ParameterValue compute(const float *samples, size_t count,
                         double sampleRate) const override {
    if (count == 0)
      return 0.0;

    double sum = 0.0;
    for (size_t i = 0; i < count; ++i) {
      const double s = static_cast<double>(samples[i]);
      sum += s * s;
    }
    return std::sqrt(sum / static_cast<double>(count));
  }
};
