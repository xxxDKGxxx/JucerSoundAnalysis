#pragma once

#include "MenuModel.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <memory>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent : public juce::Component {
public:
  //==============================================================================
  MainComponent();
  ~MainComponent() override;

  //==============================================================================
  void paint(juce::Graphics &) override;
  void resized() override;

private:
  //==============================================================================
  // Your private member variables go here...

  void loadWavFile();
  void setMenuBarBounds();

  std::unique_ptr<juce::MenuBarComponent> pMenuBarComponent;

  MenuModel menuModel;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
