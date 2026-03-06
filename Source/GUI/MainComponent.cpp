#include "MainComponent.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_audio_formats/juce_audio_formats.h"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <memory>

//==============================================================================
MainComponent::MainComponent() {
  setSize(1200, 800);

  menuModel.onLoadSoundFileClick = [this]() { this->loadWavFile(); };

  pMenuBarComponent =
      std::make_unique<juce::MenuBarComponent>(&this->menuModel);

  setMenuBarBounds();

  addAndMakeVisible(*this->pMenuBarComponent);

  audioFormatManager.registerBasicFormats();
}

MainComponent::~MainComponent() {}

//==============================================================================
void MainComponent::paint(juce::Graphics &g) {
  // (Our component is opaque, so we must completely fill the background with a
  // solid colour)
  g.fillAll(
      getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

  g.setFont(juce::FontOptions(16.0f));
  g.setColour(juce::Colours::white);
}

void MainComponent::resized() {
  // This is called when the MainComponent is resized.
  // If you add any child components, this is where you should
  // update their positions.

  if (pMenuBarComponent == nullptr) {
    return;
  }

  setMenuBarBounds();
}

void MainComponent::loadWavFile() {
  pFileChooser.reset(
      new juce::FileChooser("Select a Wave file...", juce::File{}, "*.wav"));

  auto chooserFlags = juce::FileBrowserComponent::openMode |
                      juce::FileBrowserComponent::canSelectFiles;

  pFileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser &fc) {
    auto file = fc.getResult();

    if (file == juce::File{})
      return;

    auto audioFormatReader = audioFormatManager.createReaderFor(file);

    if (audioFormatReader == nullptr)
      return;

    auto sampleRate = audioFormatReader->sampleRate;
    auto bitsPerSample = audioFormatReader->bitsPerSample;
    auto numChannels = audioFormatReader->numChannels;
    auto lengthInSamples = audioFormatReader->lengthInSamples;

    juce::Logger::writeToLog("Wczytano plik! Sample Rate: " +
                             juce::String(sampleRate));
    juce::Logger::writeToLog("Kanaly: " + juce::String(numChannels) +
                             ", Dlugosc: " + juce::String(lengthInSamples));

    auto pAudioBuffer = std::make_unique<juce::AudioBuffer<float>>(
        numChannels, lengthInSamples);

    audioFormatReader->read(pAudioBuffer.get(), 0, lengthInSamples, 0, true,
                            true);

    pAudioModel.reset(new AudioModel(std::move(pAudioBuffer), sampleRate,
                                     bitsPerSample, numChannels,
                                     lengthInSamples));
  });
}

void MainComponent::setMenuBarBounds() {
  auto area = getLocalBounds();

  pMenuBarComponent->setBounds(
      area.removeFromTop(getLookAndFeel().getDefaultMenuBarHeight()));
}
