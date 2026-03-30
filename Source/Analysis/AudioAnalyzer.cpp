#include "AudioAnalyzer.h"

#include "Parameters/AMDFF0Parameter.h"
#include "Parameters/AMDFParameter.h"
#include "Parameters/AutocorrelationF0Parameter.h"
#include "Parameters/AutocorrelationParameter.h"
#include "Parameters/IsVoicedParameter.h"
#include "Parameters/STEParameter.h"
#include "Parameters/SilenceDetectorParameter.h"
#include "Parameters/SilentRatioParameter.h"
#include "Parameters/VolumeParameter.h"
#include "Parameters/ZCRParameter.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <numeric>
#include <stdexcept>
#include <variant>

namespace {

double computeMean(const std::vector<double> &values) {
  if (values.empty())
    return 0.0;

  const double sum =
      std::accumulate(values.begin(), values.end(), 0.0, std::plus<double>());
  return sum / static_cast<double>(values.size());
}

double computeStdDev(const std::vector<double> &values) {
  if (values.size() < 2)
    return 0.0;

  const double mean = computeMean(values);
  double variance = 0.0;

  for (double value : values) {
    const double diff = value - mean;
    variance += diff * diff;
  }

  variance /= static_cast<double>(values.size());
  return std::sqrt(variance);
}

double computeRatioByWindowThreshold(const std::vector<double> &values,
                                     size_t windowSize, double multiplier,
                                     bool lessThanThreshold) {
  if (values.empty())
    return 0.0;

  const size_t effectiveWindow = std::max<size_t>(1, windowSize);
  size_t positiveCount = 0;

  for (size_t i = 0; i < values.size(); ++i) {
    const size_t begin = (i + 1 > effectiveWindow) ? (i + 1 - effectiveWindow)
                                                    : 0;
    const size_t count = i - begin + 1;

    double windowSum = 0.0;
    for (size_t k = begin; k <= i; ++k)
      windowSum += values[k];

    const double windowAverage = windowSum / static_cast<double>(count);
    const double threshold = multiplier * windowAverage;

    const bool isPositive = lessThanThreshold ? (values[i] < threshold)
                                              : (values[i] > threshold);
    if (isPositive)
      ++positiveCount;
  }

  return static_cast<double>(positiveCount) / static_cast<double>(values.size());
}

std::vector<double>
computeStdDevTimeline(const std::vector<double> &values, size_t windowSize) {
  std::vector<double> timeline;
  if (values.empty())
    return timeline;

  const size_t effectiveWindow = std::max<size_t>(1, windowSize);
  timeline.reserve(values.size());

  for (size_t i = 0; i < values.size(); ++i) {
    const size_t begin = (i + 1 > effectiveWindow) ? (i + 1 - effectiveWindow)
                                                    : 0;

    std::vector<double> window(values.begin() + static_cast<long>(begin),
                               values.begin() + static_cast<long>(i + 1));
    timeline.push_back(computeStdDev(window));
  }

  return timeline;
}

std::vector<double> computeVdrTimeline(const std::vector<double> &values,
                                       size_t windowSize) {
  constexpr double eps = 1e-12;

  std::vector<double> timeline;
  if (values.empty())
    return timeline;

  const size_t effectiveWindow = std::max<size_t>(1, windowSize);
  timeline.reserve(values.size());

  for (size_t i = 0; i < values.size(); ++i) {
    const size_t begin = (i + 1 > effectiveWindow) ? (i + 1 - effectiveWindow)
                                                    : 0;

    double minValue = values[begin];
    double maxValue = values[begin];
    for (size_t k = begin + 1; k <= i; ++k) {
      minValue = std::min(minValue, values[k]);
      maxValue = std::max(maxValue, values[k]);
    }

    const double vdr = (std::abs(maxValue) > eps)
                           ? ((maxValue - minValue) / maxValue)
                           : 0.0;
    timeline.push_back(vdr);
  }

  return timeline;
}

std::vector<double>
computeVstdTimeline(const std::vector<double> &values, size_t windowSize) {
  constexpr double eps = 1e-12;

  std::vector<double> timeline;
  if (values.empty())
    return timeline;

  const size_t effectiveWindow = std::max<size_t>(1, windowSize);
  timeline.reserve(values.size());

  for (size_t i = 0; i < values.size(); ++i) {
    const size_t begin = (i + 1 > effectiveWindow) ? (i + 1 - effectiveWindow)
                                                    : 0;

    std::vector<double> window(values.begin() + static_cast<long>(begin),
                               values.begin() + static_cast<long>(i + 1));

    const auto maxIt = std::max_element(window.begin(), window.end());
    const double maxValue = (maxIt != window.end()) ? *maxIt : 0.0;
    const double normalized =
        computeStdDev(window) / ((std::abs(maxValue) > eps) ? maxValue : 1.0);
    timeline.push_back(normalized);
  }

  return timeline;
}

std::vector<double> computeRatioTimeline(const std::vector<double> &values,
                                         size_t windowSize, double multiplier,
                                         bool lessThanThreshold) {
  std::vector<double> timeline;
  if (values.empty())
    return timeline;

  const size_t effectiveWindow = std::max<size_t>(1, windowSize);
  timeline.reserve(values.size());

  for (size_t i = 0; i < values.size(); ++i) {
    const size_t begin = (i + 1 > effectiveWindow) ? (i + 1 - effectiveWindow)
                                                    : 0;
    const size_t count = i - begin + 1;

    double windowSum = 0.0;
    for (size_t k = begin; k <= i; ++k)
      windowSum += values[k];

    const double windowAverage = windowSum / static_cast<double>(count);
    const double threshold = multiplier * windowAverage;

    size_t positiveCount = 0;
    for (size_t k = begin; k <= i; ++k) {
      const bool isPositive = lessThanThreshold ? (values[k] < threshold)
                                                : (values[k] > threshold);
      if (isPositive)
        ++positiveCount;
    }

    timeline.push_back(static_cast<double>(positiveCount) /
                       static_cast<double>(count));
  }

  return timeline;
}

double computeEnergyEntropyWindow(const std::vector<double> &energies,
                                  size_t begin, size_t end) {
  constexpr double eps = 1e-12;

  double sumEnergy = 0.0;
  for (size_t i = begin; i <= end; ++i) {
    sumEnergy += std::max(0.0, energies[i]);
  }

  if (sumEnergy <= eps)
    return 0.0;

  double entropy = 0.0;
  for (size_t i = begin; i <= end; ++i) {
    const double p = std::max(0.0, energies[i]) / sumEnergy;
    if (p > eps) {
      entropy -= p * std::log2(p);
    }
  }

  return entropy;
}

std::vector<double> computeEnergyEntropyTimeline(const std::vector<double> &energies,
                                                 size_t windowSize) {
  std::vector<double> timeline;
  if (energies.empty())
    return timeline;

  const size_t effectiveWindow = std::max<size_t>(1, windowSize);
  timeline.reserve(energies.size());

  for (size_t i = 0; i < energies.size(); ++i) {
    const size_t begin = (i + 1 > effectiveWindow) ? (i + 1 - effectiveWindow)
                                                    : 0;
    timeline.push_back(computeEnergyEntropyWindow(energies, begin, i));
  }

  return timeline;
}

double computeVuWindow(const std::vector<double> &values, size_t begin,
                       size_t end) {
  if (end <= begin + 1)
    return 0.0;

  std::vector<double> extrema;
  extrema.reserve(end - begin + 1);

  for (size_t i = begin + 1; i < end; ++i) {
    const double d1 = values[i] - values[i - 1];
    const double d2 = values[i + 1] - values[i];
    const bool isPeak = (d1 > 0.0 && d2 < 0.0);
    const bool isValley = (d1 < 0.0 && d2 > 0.0);

    if (isPeak || isValley) {
      extrema.push_back(values[i]);
    }
  }

  if (extrema.size() < 2)
    return 0.0;

  double accumulation = 0.0;
  for (size_t i = 1; i < extrema.size(); ++i) {
    accumulation += std::abs(extrema[i] - extrema[i - 1]);
  }

  return accumulation / static_cast<double>(extrema.size() - 1);
}

std::vector<double> computeVuTimeline(const std::vector<double> &values,
                                      size_t windowSize) {
  std::vector<double> timeline;
  if (values.empty())
    return timeline;

  const size_t effectiveWindow = std::max<size_t>(1, windowSize);
  timeline.reserve(values.size());

  for (size_t i = 0; i < values.size(); ++i) {
    const size_t begin = (i + 1 > effectiveWindow) ? (i + 1 - effectiveWindow)
                                                    : 0;
    timeline.push_back(computeVuWindow(values, begin, i));
  }

  return timeline;
}

} // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void AudioAnalyzer::addParameter(std::unique_ptr<IAudioParameter> param) {
  if (param) {
    parameters_.push_back(std::move(param));
  }
}

AnalysisResult AudioAnalyzer::analyze(const float *const *channelData,
                                      int numChannels, size_t numSamples,
                                      double sampleRate,
                                      const AnalysisParams &params) const {
  if (channelData == nullptr)
    throw std::invalid_argument("AudioAnalyzer::analyze – channelData is null");
  if (numChannels < 1 || numChannels > 2)
    throw std::invalid_argument(
        "AudioAnalyzer::analyze – numChannels must be 1 or 2");
  if (numSamples == 0)
    throw std::invalid_argument("AudioAnalyzer::analyze – numSamples is 0");

  const size_t frameSize = params.frameSize;
  const size_t hopSize = (params.hopSize == 0) ? frameSize : params.hopSize;
  const double clipWindowSeconds = params.clipWindowSeconds;

  if (frameSize == 0)
    throw std::invalid_argument("AudioAnalyzer::analyze – frameSize is 0");
  if (clipWindowSeconds <= 0.0)
    throw std::invalid_argument(
        "AudioAnalyzer::analyze – clipWindowSeconds must be > 0");

  AnalysisResult result;
  result.sampleRate = sampleRate;
  result.numChannels = numChannels;
  result.frameSize = frameSize;
  result.hopSize = hopSize;
  result.clipWindowSeconds = clipWindowSeconds;

  result.channels.resize(static_cast<size_t>(numChannels));

  for (int ch = 0; ch < numChannels; ++ch) {
    const float *data = channelData[ch];
    if (data == nullptr)
      throw std::invalid_argument(
          "AudioAnalyzer::analyze – channel pointer is null");

    auto &channelResult = result.channels[static_cast<size_t>(ch)];
    channelResult.channelIndex = ch;

    size_t frameIndex = 0;
    for (size_t start = 0; start + frameSize <= numSamples; start += hopSize) {
      channelResult.frames.push_back(
          analyzeFrame(data + start, frameSize, frameIndex, start, sampleRate));
      ++frameIndex;
    }

    // Precompute parameter vectors for easier plotting
    if (!channelResult.frames.empty()) {
      for (const auto &param : parameters_) {
        const std::string &name = param->getName();
        // Check first frame to see what type of parameter it is
        const auto &firstVal = channelResult.frames[0].values.at(name);

        if (std::holds_alternative<double>(firstVal)) {
          std::vector<double> vec;
          vec.reserve(channelResult.frames.size());
          for (const auto &frame : channelResult.frames) {
            vec.push_back(std::get<double>(frame.values.at(name)));
          }
          channelResult.precomputedFloatParameters[name] = std::move(vec);
        } else if (std::holds_alternative<bool>(firstVal)) {
          std::vector<bool> vec;
          vec.reserve(channelResult.frames.size());
          for (const auto &frame : channelResult.frames) {
            vec.push_back(std::get<bool>(frame.values.at(name)));
          }
          channelResult.precomputedBoolParameters[name] = std::move(vec);
        } else if (std::holds_alternative<std::optional<double>>(firstVal)) {
          std::vector<std::optional<double>> vec;
          vec.reserve(channelResult.frames.size());
          for (const auto &frame : channelResult.frames) {
            vec.push_back(
                std::get<std::optional<double>>(frame.values.at(name)));
          }
          channelResult.precomputedOptionalFloatParameters[name] =
              std::move(vec);
        }
      }

      computeClipLevelParameters(channelResult, sampleRate, hopSize,
                                 clipWindowSeconds);
    }
  }

  return result;
}

void AudioAnalyzer::computeClipLevelParameters(ChannelAnalysisResult &channelResult,
                                               double sampleRate,
                                               size_t hopSize,
                                               double clipWindowSeconds) const {
  constexpr double eps = 1e-12;

  if (sampleRate <= 0.0 || hopSize == 0)
    return;

  const double frameStepSeconds = static_cast<double>(hopSize) / sampleRate;
  size_t clipWindowFrames = static_cast<size_t>(
      std::round(clipWindowSeconds / std::max(frameStepSeconds, 1e-12)));
  clipWindowFrames = std::max<size_t>(1, clipWindowFrames);

  const auto volIt =
      channelResult.precomputedFloatParameters.find("volume");
  const auto steIt =
      channelResult.precomputedFloatParameters.find("shortTimeEnergy");
  const auto zcrIt =
      channelResult.precomputedFloatParameters.find("zeroCrossingRate");

  if (volIt != channelResult.precomputedFloatParameters.end()) {
    const auto &volume = volIt->second;

    if (!volume.empty()) {
      const auto [minIt, maxIt] =
          std::minmax_element(volume.begin(), volume.end());
      const double maxVolume = *maxIt;
      const double minVolume = *minIt;

      const double vstd = computeStdDev(volume) /
                          ((std::abs(maxVolume) > eps) ? maxVolume : 1.0);

      const double vdr = (std::abs(maxVolume) > eps)
                             ? ((maxVolume - minVolume) / maxVolume)
                             : 0.0;

      channelResult.clipFloatParameters["VSTD"] = vstd;
      channelResult.clipFloatParameters["VDR"] = vdr;
      channelResult.clipTimeSeriesFloatParameters["VSTD"] =
          computeVstdTimeline(volume, clipWindowFrames);
      channelResult.clipTimeSeriesFloatParameters["VDR"] =
          computeVdrTimeline(volume, clipWindowFrames);

        auto vuTimeline = computeVuTimeline(volume, clipWindowFrames);
        channelResult.clipFloatParameters["VU"] = computeMean(vuTimeline);
        channelResult.clipTimeSeriesFloatParameters["VU"] =
          std::move(vuTimeline);
    }
  }

  if (zcrIt != channelResult.precomputedFloatParameters.end()) {
    const auto &zcr = zcrIt->second;
    if (!zcr.empty()) {
      channelResult.clipFloatParameters["ZSTD"] = computeStdDev(zcr);
      channelResult.clipTimeSeriesFloatParameters["ZSTD"] =
          computeStdDevTimeline(zcr, clipWindowFrames);
    }
  }

  if (steIt != channelResult.precomputedFloatParameters.end()) {
    const auto &ste = steIt->second;
    if (!ste.empty()) {
      channelResult.clipFloatParameters["LSTER"] =
        computeRatioByWindowThreshold(ste, clipWindowFrames, 0.5, true);
      channelResult.clipTimeSeriesFloatParameters["LSTER"] =
        computeRatioTimeline(ste, clipWindowFrames, 0.5, true);

      auto entropyTimeline = computeEnergyEntropyTimeline(ste, clipWindowFrames);
      channelResult.clipFloatParameters["EnergyEntropy"] =
        computeMean(entropyTimeline);
      channelResult.clipTimeSeriesFloatParameters["EnergyEntropy"] =
        std::move(entropyTimeline);
    }
  }

  if (zcrIt != channelResult.precomputedFloatParameters.end()) {
    const auto &zcr = zcrIt->second;
    if (!zcr.empty()) {
      channelResult.clipFloatParameters["HZCRR"] = computeRatioByWindowThreshold(
          zcr, clipWindowFrames, 1.5, false);
      channelResult.clipTimeSeriesFloatParameters["HZCRR"] =
          computeRatioTimeline(zcr, clipWindowFrames, 1.5, false);
    }
  }
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

FrameResult AudioAnalyzer::analyzeFrame(const float *samples, size_t frameSize,
                                        size_t frameIndex, size_t startSample,
                                        double sampleRate) const {
  FrameResult fr;
  fr.frameIndex = frameIndex;
  fr.startSample = startSample;
  fr.endSample = startSample + frameSize;

  // Każdy zarejestrowany parametr sam oblicza swoją wartość (SRP).
  for (const auto &param : parameters_) {
    fr.values[param->getName()] =
        param->compute(samples, frameSize, sampleRate);
  }

  return fr;
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

AudioAnalyzer AudioAnalyzer::createDefault(double silenceVolumeThreshold,
                                           double silenceZcrThreshold,
                                           double voicedZcrThreshold,
                                           double voicedSteThreshold,
                                           bool computeAutocorrelation,
                                           bool computeAmdf) {
  AudioAnalyzer analyzer;

  analyzer.addParameter(std::make_unique<VolumeParameter>());
  analyzer.addParameter(std::make_unique<STEParameter>());
  analyzer.addParameter(std::make_unique<ZCRParameter>());
  analyzer.addParameter(std::make_unique<SilentRatioParameter>(
      silenceVolumeThreshold, silenceZcrThreshold));
  analyzer.addParameter(std::make_unique<SilenceDetectorParameter>(
      silenceVolumeThreshold, silenceZcrThreshold));
  analyzer.addParameter(std::make_unique<IsVoicedParameter>(
      voicedZcrThreshold, voicedSteThreshold));
  analyzer.addParameter(std::make_unique<AutocorrelationF0Parameter>());
  analyzer.addParameter(std::make_unique<AMDFF0Parameter>());

  if (computeAutocorrelation)
    analyzer.addParameter(std::make_unique<AutocorrelationParameter>());

  if (computeAmdf)
    analyzer.addParameter(std::make_unique<AMDFParameter>());

  return analyzer;
}
