#include "AudioAnalyzer.h"

#include "Parameters/AMDFParameter.h"
#include "Parameters/AutocorrelationParameter.h"
#include "Parameters/IsVoicedParameter.h"
#include "Parameters/STEParameter.h"
#include "Parameters/SilenceDetectorParameter.h"
#include "Parameters/SilentRatioParameter.h"
#include "Parameters/VolumeParameter.h"
#include "Parameters/ZCRParameter.h"

#include <memory>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void AudioAnalyzer::addParameter(std::unique_ptr<IAudioParameter> param) {
  if (param)
    parameters_.push_back(std::move(param));
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

  if (frameSize == 0)
    throw std::invalid_argument("AudioAnalyzer::analyze – frameSize is 0");

  AnalysisResult result;
  result.sampleRate = sampleRate;
  result.numChannels = numChannels;
  result.frameSize = frameSize;
  result.hopSize = hopSize;

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
          analyzeFrame(data + start, frameSize, frameIndex, start));
      ++frameIndex;
    }
  }

  return result;
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

FrameResult AudioAnalyzer::analyzeFrame(const float *samples, size_t frameSize,
                                        size_t frameIndex,
                                        size_t startSample) const {
  FrameResult fr;
  fr.frameIndex = frameIndex;
  fr.startSample = startSample;
  fr.endSample = startSample + frameSize;

  // Każdy zarejestrowany parametr sam oblicza swoją wartość (SRP).
  for (const auto &param : parameters_) {
    fr.values[param->getName()] = param->compute(samples, frameSize);
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

  if (computeAutocorrelation)
    analyzer.addParameter(std::make_unique<AutocorrelationParameter>());

  if (computeAmdf)
    analyzer.addParameter(std::make_unique<AMDFParameter>());

  return analyzer;
}
