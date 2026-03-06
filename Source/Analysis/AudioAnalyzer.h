#pragma once

#include <cstddef>
#include <string>
#include <vector>

struct FrameResult {
    size_t frameIndex = 0;  // sequential frame number
    size_t startSample = 0; // first sample of this frame in the buffer
    size_t endSample = 0; // one-past-last sample of this frame

    double volume = 0.0; 
    double shortTimeEnergy = 0.0;    
    double zeroCrossingRate = 0.0; // normalised [0,1]
    double silentRatio = 0.0;    // ratio of "silent" samples in the frame [0,1]
    bool   isSilent = false;

    // Autocorrelation values for lags 0..frameSize-1
    std::vector<double> autocorrelation;

    // AMDF (Average Magnitude Difference Function) for lags 0..frameSize-1
    std::vector<double> amdf;
};

// Complete analysis result for one audio channel
struct ChannelAnalysisResult {
    int    channelIndex = 0;
    std::vector<FrameResult> frames;
};

// Full analysis output – one entry per channel.
struct AnalysisResult {
    double sampleRate = 0.0;
    int    numChannels = 0;
    size_t frameSize = 0;
    size_t hopSize = 0; 
    std::vector<ChannelAnalysisResult> channels;
};


struct AnalysisParams {
    size_t frameSize = 1024;
    size_t hopSize = 0;

    double silenceVolumeThreshold = 0.01; /* We have to experiment with this value and adjust it.
    Moreover we have to put our experminets results in the report.
    */

    // Frames with ZCR > this AND volume < silenceVolumeThreshold are silent.
    double silenceZcrThreshold = 0.1;
    /* We have to experiment with this value and adjust it.
    Moreover we have to put our experminets results in the report.
    */


    // Amplitude below which a single sample is considered "silent"
    // (used by Silent Ratio calculation).
    double sampleSilenceThreshold = 0.02;

    // Whether to compute autocorrelation per frame (can be expensive)
    bool computeAutocorrelation = true;
    bool computeAmdf            = true;
};


class AudioAnalyzer {
public:
    AudioAnalyzer() = default;
    ~AudioAnalyzer() = default;

    AnalysisResult analyze(const float* const* channelData,
                           int numChannels,
                           size_t numSamples,
                           double sampleRate,
                           const AnalysisParams& params = {}) const;


    static double computeVolume(const float* samples, size_t count);

    // Short-Time Energy
    static double computeSTE(const float* samples, size_t count);

    // Zero Crossing Rate normalised to [0, 1].
    static double computeZCR(const float* samples, size_t count);

    static double computeSilentRatio(const float* samples, size_t count,
                                     double threshold);

    static std::vector<double> computeAutocorrelation(const float* samples,
                                                      size_t count,
                                                      size_t maxLag);

    static std::vector<double> computeAMDF(const float* samples,
                                           size_t count,
                                           size_t maxLag);

    static bool detectSilence(double volume, double zcr,
                              double volumeThreshold,
                              double zcrThreshold);

private:
    FrameResult analyzeFrame(const float* samples,
                             size_t frameSize,
                             size_t frameIndex,
                             size_t startSample,
                             const AnalysisParams& params) const;
};
