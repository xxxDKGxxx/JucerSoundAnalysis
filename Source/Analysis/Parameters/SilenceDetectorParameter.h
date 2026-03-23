#pragma once

#include "IAudioParameter.h"
#include "SilentRatioParameter.h"

// zostawiam bo mozna dodac dodatkowe parametry wykrywania ciszy w przyszlosci
class SilenceDetectorParameter : public IAudioParameter {
public:
  SilenceDetectorParameter(double volumeThreshold = 0.01,
                           double zcrThreshold = 0.1)
      : silentRatioParam_(volumeThreshold, zcrThreshold) {}

  std::string getName() const override { return "isSilent"; }

  ParameterValue compute(const float *samples, size_t count,
                         double sampleRate) const override {
    return silentRatioParam_.compute(samples, count, sampleRate);
  }

private:
  SilentRatioParameter silentRatioParam_;
};
