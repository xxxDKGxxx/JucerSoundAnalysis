#include "AMDFParameter.h"
#include "IAudioParameter.h"
#include "IsVoicedParameter.h"

class AMDFF0Parameter : public IAudioParameter {
public:
  std::string getName() const override { return "AMDFF0(kHz)"; };
  ParameterValue compute(const float *samples, size_t count,
                         double sampleRate) const override {
    auto isVoiced =
        std::get<bool>(isVoicedParam_.compute(samples, count, sampleRate));

    if (!isVoiced) {
      return std::nullopt;
    }

    auto amdf = std::get<std::vector<double>>(
        amdfParam_.compute(samples, count, sampleRate));

    auto minLag = 0;
    auto isDecreasing = false;

    for (auto i = 1; i < amdf.size(); i++) {
      if (amdf[i] < amdf[i - 1]) {
        isDecreasing = true;
        continue;
      }

      if (isDecreasing && amdf[i] > amdf[i - 1]) {
        minLag = i - 1;
        break;
      } // found local min
    }

    if (minLag == 0) {
      return std::nullopt;
    }

    auto frequency = sampleRate / minLag / 1000.0;

    return std::optional(frequency);
  }

private:
  IsVoicedParameter isVoicedParam_;
  AMDFParameter amdfParam_;
};
