#include "AudioModel.h"

AudioModel::AudioModel(juce::AudioBuffer<float> *audioBuffer, double sampleRate,
                       unsigned int bitsPerSample, unsigned int numChannels,
                       juce::int64 lengthInSamples)
    : audioBuffer(audioBuffer), sampleRate(sampleRate),
      bitsPerSample(bitsPerSample), numChannels(numChannels),
      lengthInSamples(lengthInSamples) {}
juce::AudioBuffer<float> AudioModel::getAudioBuffer() { return *audioBuffer; }
double AudioModel::getSampleRate() { return sampleRate; };
unsigned int AudioModel::getBitsPerSample() { return bitsPerSample; };
unsigned int AudioModel::getNumChannels() { return numChannels; };
long int AudioModel::getLengthInSamples() { return lengthInSamples; };
