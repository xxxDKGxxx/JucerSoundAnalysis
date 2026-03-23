#pragma once

#include "IAudioParameter.h"
#include "STEParameter.h"
#include "ZCRParameter.h"

class IsVoicedParameter : public IAudioParameter {
public:
  IsVoicedParameter(double zcrThreshold = 0.1, double steThreshold = 0.002)
      : zcrThreshold(zcrThreshold), steThreshold(steThreshold) {}

  std::string getName() const override { return "isVoiced"; }

  ParameterValue compute(const float *samples, size_t count,
                         double sampleRate) const override {
    auto zcr = std::get<double>(zcrParam_.compute(samples, count, sampleRate));
    auto ste = std::get<double>(steParam_.compute(samples, count, sampleRate));

    return zcr < zcrThreshold && ste > steThreshold;
  }

private:
  ZCRParameter zcrParam_;
  STEParameter steParam_;

  double zcrThreshold;
  double steThreshold;
};
