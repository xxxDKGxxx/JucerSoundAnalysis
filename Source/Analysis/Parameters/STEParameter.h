#pragma once

#include "IAudioParameter.h"
#include "VolumeParameter.h"

class STEParameter : public IAudioParameter {
public:
    std::string getName() const override { return "shortTimeEnergy"; }

    ParameterValue compute(const float* samples, size_t count) const override {
        double volume = std::get<double>(volumeParam_.compute(samples, count));
        return volume * volume;
    }

private:
    VolumeParameter volumeParam_;
};
