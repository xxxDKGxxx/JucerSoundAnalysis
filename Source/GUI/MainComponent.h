#pragma once

#include "../Analysis/AudioAnalyzer.h"
#include "Menu/MenuModel.h"
#include "Model/AudioModel.h"
#include "Panels/FrameParametersPanel.h"
#include "Panels/HeaderPanel.h"
#include "Panels/WaveformPanel.h"
#include "juce_audio_formats/juce_audio_formats.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <juce_opengl/juce_opengl.h>

#include <map>
#include <memory>

#include <imgui.h>
#include <imgui_impl_juce/imgui_impl_juce.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put
   all your controls and content.
*/
class MainComponent : public juce::Component, juce::OpenGLRenderer {
public:
  //==============================================================================
  MainComponent();
  ~MainComponent() override;

  //==============================================================================
  void paint(juce::Graphics &) override;
  void resized() override;

  void newOpenGLContextCreated() override;

  void renderOpenGL() override;

  void openGLContextClosing() override;

private:
  //==============================================================================
  // Your private member variables go here...

  void loadWavFile();
  void reanalyzeCurrentAudio();
  void setMenuBarBounds();

  std::unique_ptr<juce::MenuBarComponent> pMenuBarComponent;
  std::unique_ptr<AudioModel> pAudioModel;
  std::unique_ptr<juce::FileChooser> pFileChooser;

  std::vector<std::pair<std::string, ParameterType>> frameParameters;
  std::map<std::pair<std::string, ParameterType>, bool> chosenFrameParameters;

  MenuModel menuModel;
  HeaderPanel headerPanel;
  WaveformPanel waveformPanel;
  FrameParametersPanel frameParametersPanel;

  AudioAnalyzer audioAnalyzer;
  AnalysisResult analysisResult;
  juce::CriticalSection dataLock;

  std::atomic<bool> isAnalysisRunning{false};
  std::string statusMessage;
  int selectedFrameSize = 1024;

  unsigned int imguiOffsetY;

  juce::AudioFormatManager audioFormatManager;
  juce::OpenGLContext glctx;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
