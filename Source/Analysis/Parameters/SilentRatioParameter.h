#pragma once

#include "IAudioParameter.h"
#include "VolumeParameter.h"
#include "ZCRParameter.h"

/// Silent Ratio — klasyfikuje ramkę jako ciszę gdy Volume < volumeThreshold
/// ORAZ ZCR < zcrThreshold.  Zwraca bool (true = ramka jest cicha).
class SilentRatioParameter : public IAudioParameter {
public:
  SilentRatioParameter(double volumeThreshold = 0.01, double zcrThreshold = 0.1)
      : volumeThreshold_(volumeThreshold), zcrThreshold_(zcrThreshold) {}

  std::string getName() const override { return "silentRatio"; }

  ParameterValue compute(const float *samples, size_t count,
                         double sampleRate) const override {
    double volume =
        std::get<double>(volumeParam_.compute(samples, count, sampleRate));
    double zcr =
        std::get<double>(zcrParam_.compute(samples, count, sampleRate));
    return (volume < volumeThreshold_ && zcr < zcrThreshold_);
  }

private:
  double volumeThreshold_;
  double zcrThreshold_;

  VolumeParameter volumeParam_;
  ZCRParameter zcrParam_;
};
