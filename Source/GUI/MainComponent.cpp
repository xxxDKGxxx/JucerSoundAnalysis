#include "MainComponent.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <memory>

//==============================================================================
MainComponent::MainComponent() {
  setSize(1200, 800);

  this->menuModel.onLoadSoundFileClick = [this]() { this->loadWavFile(); };

  this->pMenuBarComponent =
      std::make_unique<juce::MenuBarComponent>(&this->menuModel);

  setMenuBarBounds();

  addAndMakeVisible(*this->pMenuBarComponent);
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

void MainComponent::loadWavFile() { std::cout << "Loading file...\n"; }

void MainComponent::setMenuBarBounds() {
  auto area = getLocalBounds();

  pMenuBarComponent->setBounds(
      area.removeFromTop(getLookAndFeel().getDefaultMenuBarHeight()));
}
