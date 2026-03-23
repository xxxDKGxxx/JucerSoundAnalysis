#include "AutocorrelationParameter.h"
#include "IAudioParameter.h"
#include "IsVoicedParameter.h"
#include <optional>

class AutocorrelationF0Parameter : public IAudioParameter {
public:
  std::string getName() const override { return "AutocorrelationF0(kHz)"; };
  ParameterValue compute(const float *samples, size_t count,
                         double sampleRate) const override {
    auto isVoiced =
        std::get<bool>(isVoicedParam_.compute(samples, count, sampleRate));

    if (!isVoiced) {
      return std::nullopt;
    }

    auto autocorrelation = std::get<std::vector<double>>(
        autocorrelationParam_.compute(samples, count, sampleRate));

    auto maxLag = 0;
    auto isIncreasing = false;

    for (auto i = 1; i < autocorrelation.size(); i++) {
      if (autocorrelation[i] > autocorrelation[i - 1]) {
        isIncreasing = true;
        continue;
      }

      if (isIncreasing && autocorrelation[i] < autocorrelation[i - 1]) {
        maxLag = i - 1;
        break;
      } // found local max
    }

    if (maxLag == 0) {
      return std::nullopt;
    }

    auto frequency = sampleRate / maxLag / 1000.0;

    return std::optional(frequency);
  }

private:
  IsVoicedParameter isVoicedParam_;
  AutocorrelationParameter autocorrelationParam_;
};
