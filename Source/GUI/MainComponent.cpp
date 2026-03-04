#include "MainComponent.h"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <memory>

//==============================================================================
MainComponent::MainComponent() {
  setSize(1200, 800);

  this->openButton =
      std::make_unique<juce::TextButton>(juce::CharPointer_UTF8("Otwórz plik"));

  addAndMakeVisible(*this->openButton);

  this->openButton->setBounds(50, 50, 150, 40);
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
}
