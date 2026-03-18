#include "IAudioParameter.h"
#include "STEParameter.h"
#include "ZCRParameter.h"

class IsVoicedParameter : public IAudioParameter {
public:
  IsVoicedParameter(double zcrThreshold = 0.1, double steThreshold = 0.1)
      : zcrThreshold(zcrThreshold), steThreshold(steThreshold) {}

  std::string getName() const override { return "isVoiced"; }

  ParameterValue compute(const float *samples, size_t count) const override {
    auto zcr = std::get<double>(zcrParam_.compute(samples, count));
    auto ste = std::get<double>(steParam_.compute(samples, count));

    return zcr < zcrThreshold && ste > steThreshold;
  }

private:
  ZCRParameter zcrParam_;
  STEParameter steParam_;

  double zcrThreshold;
  double steThreshold;
};
