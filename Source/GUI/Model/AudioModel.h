#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include <memory>

class AudioModel {
public:
  AudioModel(juce::AudioBuffer<float> *audioBuffer, double sampleRate,
             unsigned int bitsPerSample, unsigned int numChannels,
             juce::int64 lengthInSamples);

  juce::AudioBuffer<float> getAudioBuffer();
  double getSampleRate();
  unsigned int getBitsPerSample();
  unsigned int getNumChannels();
  long int getLengthInSamples();

private:
  std::unique_ptr<juce::AudioBuffer<float>> audioBuffer;
  double sampleRate;
  unsigned int bitsPerSample;
  unsigned int numChannels;
  long int lengthInSamples;
};
