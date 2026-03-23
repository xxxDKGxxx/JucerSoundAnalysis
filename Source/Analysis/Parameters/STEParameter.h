#pragma once

#include "IAudioParameter.h"
#include "VolumeParameter.h"

class STEParameter : public IAudioParameter {
public:
  std::string getName() const override { return "shortTimeEnergy"; }

  ParameterValue compute(const float *samples, size_t count,
                         double sampleRate) const override {
    double volume =
        std::get<double>(volumeParam_.compute(samples, count, sampleRate));
    return volume * volume;
  }

private:
  VolumeParameter volumeParam_;
};
