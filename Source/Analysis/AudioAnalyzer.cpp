#include "AudioAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

AnalysisResult AudioAnalyzer::analyze(const float* const* channelData,
                                      int numChannels,
                                      size_t numSamples,
                                      double sampleRate,
                                      const AnalysisParams& params) const
{
    if (channelData == nullptr)
        throw std::invalid_argument("AudioAnalyzer::analyze – channelData is null");
    if (numChannels < 1 || numChannels > 2)
        throw std::invalid_argument("AudioAnalyzer::analyze – numChannels must be 1 or 2");
    if (numSamples == 0)
        throw std::invalid_argument("AudioAnalyzer::analyze – numSamples is 0");

    const size_t frameSize = params.frameSize;
    const size_t hopSize   = (params.hopSize == 0) ? frameSize : params.hopSize;

    if (frameSize == 0)
        throw std::invalid_argument("AudioAnalyzer::analyze – frameSize is 0");

    AnalysisResult result;
    result.sampleRate  = sampleRate;
    result.numChannels = numChannels;
    result.frameSize   = frameSize;
    result.hopSize     = hopSize;

    result.channels.resize(static_cast<size_t>(numChannels));

    for (int ch = 0; ch < numChannels; ++ch) {
        const float* data = channelData[ch];
        if (data == nullptr)
            throw std::invalid_argument("AudioAnalyzer::analyze – channel pointer is null");

        auto& channelResult = result.channels[static_cast<size_t>(ch)];
        channelResult.channelIndex = ch;

        size_t frameIndex = 0;
        for (size_t start = 0; start + frameSize <= numSamples; start += hopSize) {
            channelResult.frames.push_back(
                analyzeFrame(data + start, frameSize, frameIndex, start, params));
            ++frameIndex;
        }
    }

    return result;
}

FrameResult AudioAnalyzer::analyzeFrame(const float* samples,
                                        size_t frameSize,
                                        size_t frameIndex,
                                        size_t startSample,
                                        const AnalysisParams& params) const
{
    FrameResult fr;
    fr.frameIndex  = frameIndex;
    fr.startSample = startSample;
    fr.endSample   = startSample + frameSize;

    fr.volume           = computeVolume(samples, frameSize);
    fr.shortTimeEnergy  = computeSTE(samples, frameSize);
    fr.zeroCrossingRate = computeZCR(samples, frameSize);
    fr.silentRatio      = computeSilentRatio(samples, frameSize,
                                             params.sampleSilenceThreshold);

    fr.isSilent = detectSilence(fr.volume, fr.zeroCrossingRate,
                                params.silenceVolumeThreshold,
                                params.silenceZcrThreshold);

    if (params.computeAutocorrelation)
        fr.autocorrelation = computeAutocorrelation(samples, frameSize, frameSize);

    if (params.computeAmdf)
        fr.amdf = computeAMDF(samples, frameSize, frameSize);

    return fr;
}


double AudioAnalyzer::computeVolume(const float* samples, size_t count)
{
    if (count == 0) return 0.0;

    double sum = 0.0;
    for (size_t i = 0; i < count; ++i) {
        const double s = static_cast<double>(samples[i]);
        sum += s * s;
    }
    return std::sqrt(sum / static_cast<double>(count));
}

double AudioAnalyzer::computeSTE(const float* samples, size_t count)
{
    if (count == 0) return 0.0;

    double sum = 0.0;
    for (size_t i = 0; i < count; ++i) {
        const double s = static_cast<double>(samples[i]);
        sum += s * s;
    }
    return sum / static_cast<double>(count);
}


double AudioAnalyzer::computeZCR(const float* samples, size_t count)
{
    if (count < 2) return 0.0;

    size_t crossings = 0;
    for (size_t i = 1; i < count; ++i) {
        if ((samples[i] >= 0.0f) != (samples[i - 1] >= 0.0f))
            ++crossings;
    }
    return static_cast<double>(crossings) / static_cast<double>(count - 1);
}

double AudioAnalyzer::computeSilentRatio(const float* samples, size_t count,
                                         double threshold)
{
    if (count == 0) return 0.0;

    size_t silentCount = 0;
    const float thr = static_cast<float>(threshold);
    for (size_t i = 0; i < count; ++i) {
        if (std::fabs(samples[i]) < thr)
            ++silentCount;
    }
    return static_cast<double>(silentCount) / static_cast<double>(count);
}

std::vector<double> AudioAnalyzer::computeAutocorrelation(const float* samples,
                                                          size_t count,
                                                          size_t maxLag)
{
    std::vector<double> result(maxLag, 0.0);
    if (count == 0) return result;

    for (size_t lag = 0; lag < maxLag; ++lag) {
        double sum = 0.0;
        const size_t limit = (lag < count) ? (count - lag) : 0;
        for (size_t n = 0; n < limit; ++n) {
            sum += static_cast<double>(samples[n])
                 * static_cast<double>(samples[n + lag]);
        }
        result[lag] = sum / static_cast<double>(count);
    }
    return result;
}

std::vector<double> AudioAnalyzer::computeAMDF(const float* samples,
                                               size_t count,
                                               size_t maxLag)
{
    std::vector<double> result(maxLag, 0.0);
    if (count == 0) return result;

    for (size_t lag = 0; lag < maxLag; ++lag) {
        const size_t limit = (lag < count) ? (count - lag) : 0;
        if (limit == 0) {
            result[lag] = 0.0;
            continue;
        }
        double sum = 0.0;
        for (size_t n = 0; n < limit; ++n) {
            sum += std::fabs(static_cast<double>(samples[n])
                           - static_cast<double>(samples[n + lag]));
        }
        result[lag] = sum / static_cast<double>(limit);
    }
    return result;
}


bool AudioAnalyzer::detectSilence(double volume, double zcr,
                                  double volumeThreshold,
                                  double zcrThreshold)
{
    // I use 2 criteria for silence detection 
    // 1st - low energy and secondaty ZCR 
    if (volume < volumeThreshold)
        return true;

    if (volume < volumeThreshold * 2.0 && zcr < zcrThreshold)
        return true;

    return false;
}
