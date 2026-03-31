#pragma once

#include "IAudioParameter.h"
#include <cmath>

class CrestFactorParameter : public IAudioParameter {
public:
  std::string getName() const override { return "crestFactor(da)"; }

  ParameterValue compute(const float *samples, size_t count,
                         double sampleRate) const override {
    if (count == 0)
      return 0.0;

    double sumSq = 0.0;
    float peak = 0.0f;

    for (size_t i = 0; i < count; ++i) {
      const float s = samples[i];
      const float absS = std::abs(s);
      if (absS > peak)
        peak = absS;
      sumSq += static_cast<double>(s) * s;
    }

    const double rms = std::sqrt(sumSq / static_cast<double>(count));

    if (rms < 1e-12)
      return 0.0;

    return static_cast<double>(peak) / rms / 10.0;
  }
};
