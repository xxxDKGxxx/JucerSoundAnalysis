#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include <memory>

class AudioModel {
public:
  AudioModel(std::unique_ptr<juce::AudioBuffer<float>> audioBuffer,
             double sampleRate, unsigned int bitsPerSample,
             unsigned int numChannels, juce::int64 lengthInSamples);

  juce::AudioBuffer<float> &getAudioBuffer();
  double getSampleRate() const;
  unsigned int getBitsPerSample() const;
  unsigned int getNumChannels() const;
  juce::int64 getLengthInSamples() const;

private:
  std::unique_ptr<juce::AudioBuffer<float>> audioBuffer;
  double sampleRate;
  unsigned int bitsPerSample;
  unsigned int numChannels;
  juce::int64 lengthInSamples;
};
