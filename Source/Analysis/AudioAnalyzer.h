#pragma once

#include "Parameters/IAudioParameter.h"

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

struct FrameResult {
  size_t frameIndex = 0;
  size_t startSample = 0;
  size_t endSample = 0;

  std::map<std::string, ParameterValue> values;

  std::optional<double> getOptionalDouble(const std::string &key) const {
    auto it = values.find(key);

    if (it != values.end() &&
        std::holds_alternative<std::optional<double>>(it->second)) {
      return std::get<std::optional<double>>(it->second);
    }

    return std::nullopt;
  }

  double getDouble(const std::string &key) const {
    auto it = values.find(key);
    if (it != values.end() && std::holds_alternative<double>(it->second))
      return std::get<double>(it->second);
    return 0.0;
  }

  bool getBool(const std::string &key) const {
    auto it = values.find(key);
    if (it != values.end() && std::holds_alternative<bool>(it->second))
      return std::get<bool>(it->second);
    return false;
  }

  const std::vector<double> &getVector(const std::string &key) const {
    static const std::vector<double> empty;
    auto it = values.find(key);
    if (it != values.end() &&
        std::holds_alternative<std::vector<double>>(it->second))
      return std::get<std::vector<double>>(it->second);
    return empty;
  }
};

struct ChannelAnalysisResult {
  int channelIndex = 0;
  std::vector<FrameResult> frames;
  std::map<std::string, std::vector<double>> precomputedFloatParameters;
  std::map<std::string, std::vector<bool>> precomputedBoolParameters;
  std::map<std::string, std::vector<std::optional<double>>>
      precomputedOptionalFloatParameters;
};

struct AnalysisResult {
  double sampleRate = 0.0;
  int numChannels = 0;
  size_t frameSize = 0;
  size_t hopSize = 0;
  std::vector<ChannelAnalysisResult> channels;
};

struct AnalysisParams {
  size_t frameSize = 1024;
  size_t hopSize =
      0; // hopSize == 0 to ramki sie nie nakladaja, hopSize == frameSize to
         // ramki sie nie nakladaja, hopSize < frameSize to ramki sie nakladaja
};

class AudioAnalyzer {
public:
  AudioAnalyzer() = default;
  ~AudioAnalyzer() = default;

  AudioAnalyzer(AudioAnalyzer &&) noexcept = default;
  AudioAnalyzer &operator=(AudioAnalyzer &&) noexcept = default;
  AudioAnalyzer(const AudioAnalyzer &) = delete;
  AudioAnalyzer &operator=(const AudioAnalyzer &) = delete;

  void addParameter(std::unique_ptr<IAudioParameter> param);

  AnalysisResult analyze(const float *const *channelData, int numChannels,
                         size_t numSamples, double sampleRate,
                         const AnalysisParams &params = {}) const;

  static AudioAnalyzer createDefault(double silenceVolumeThreshold = 0.01,
                                     double silenceZcrThreshold = 0.1,
                                     double voicedZcrThreshold = 0.1,
                                     double voicedSteThreshold = 0.0015,
                                     bool computeAutocorrelation = true,
                                     bool computeAmdf = true);

private:
  std::vector<std::unique_ptr<IAudioParameter>> parameters_;

  FrameResult analyzeFrame(const float *samples, size_t frameSize,
                           size_t frameIndex, size_t startSample,
                           double sampleRate) const;
};
