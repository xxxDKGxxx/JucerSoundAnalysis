#include "AudioModel.h"
#include "juce_core/juce_core.h"

AudioModel::AudioModel(std::unique_ptr<juce::AudioBuffer<float>> audioBuffer,
                       double sampleRate, unsigned int bitsPerSample,
                       unsigned int numChannels, juce::int64 lengthInSamples)
    : audioBuffer(std::move(audioBuffer)), sampleRate(sampleRate),
      bitsPerSample(bitsPerSample), numChannels(numChannels),
      lengthInSamples(lengthInSamples) {}
juce::AudioBuffer<float> &AudioModel::getAudioBuffer() { return *audioBuffer; }
double AudioModel::getSampleRate() const { return sampleRate; };
unsigned int AudioModel::getBitsPerSample() const { return bitsPerSample; };
unsigned int AudioModel::getNumChannels() const { return numChannels; };
juce::int64 AudioModel::getLengthInSamples() const { return lengthInSamples; };
