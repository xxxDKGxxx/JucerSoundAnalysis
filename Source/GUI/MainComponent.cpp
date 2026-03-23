#include "MainComponent.h"
#include "Panels/FrameParametersPanel.h"
#include "imgui.h"
#include "implot.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_audio_formats/juce_audio_formats.h"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <cstddef>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

//==============================================================================
MainComponent::MainComponent() {
  setSize(1200, 800);

  menuModel.onLoadSoundFileClick = [this]() { this->loadWavFile(); };

  pMenuBarComponent =
      std::make_unique<juce::MenuBarComponent>(&this->menuModel);

  setMenuBarBounds();

  addAndMakeVisible(*this->pMenuBarComponent);

  audioFormatManager.registerBasicFormats();

  glctx.setOpenGLVersionRequired(juce::OpenGLContext::openGL3_2);
  glctx.setRenderer(this);
  glctx.attachTo(*this);
  glctx.setContinuousRepainting(true);

  audioAnalyzer = AudioAnalyzer::createDefault();

  frameParameters.push_back(std::pair("zeroCrossingRate", Float));
  frameParameters.push_back(std::pair("volume", Float));
  frameParameters.push_back(std::pair("shortTimeEnergy", Float));
  frameParameters.push_back(std::pair("isSilent", Bool));
  frameParameters.push_back(std::pair("isVoiced", Bool));
  frameParameters.push_back(
      std::pair("AutocorrelationF0(kHz)", InterpolatedFloatOption));
  frameParameters.push_back(std::pair("AMDFF0(kHz)", InterpolatedFloatOption));

  for (auto frameParameterPair : frameParameters) {
    chosenFrameParameters[frameParameterPair] = false;
  }
}

MainComponent::~MainComponent() { glctx.detach(); }

//==============================================================================
void MainComponent::paint(juce::Graphics &g) {
  // (Our component is opaque, so we must completely fill the background with a
  // solid colour)
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
  if (isAnalysisRunning)
    return;

  pFileChooser.reset(
      new juce::FileChooser("Select a Wave file...", juce::File{}, "*.wav"));

  auto chooserFlags = juce::FileBrowserComponent::openMode |
                      juce::FileBrowserComponent::canSelectFiles;

  pFileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser &fc) {
    auto file = fc.getResult();

    if (file == juce::File{})
      return;

    isAnalysisRunning = true;
    statusMessage = "Loading file...";

    // Start background thread for processing
    std::thread([this, file]() {
      auto audioFormatReader = audioFormatManager.createReaderFor(file);

      if (audioFormatReader == nullptr) {
        isAnalysisRunning = false;
        return;
      }

      auto sampleRate = audioFormatReader->sampleRate;
      auto bitsPerSample = audioFormatReader->bitsPerSample;
      auto numChannels = audioFormatReader->numChannels;
      auto lengthInSamples = audioFormatReader->lengthInSamples;

      auto pNewAudioBuffer = std::make_unique<juce::AudioBuffer<float>>(
          numChannels, lengthInSamples);

      audioFormatReader->read(pNewAudioBuffer.get(), 0,
                              static_cast<int>(lengthInSamples), 0, true, true);

      auto pNewAudioModel = std::make_unique<AudioModel>(
          std::move(pNewAudioBuffer), sampleRate, bitsPerSample, numChannels,
          lengthInSamples);

      statusMessage = "Analyzing...";

      try {
        auto newAnalysisResult = audioAnalyzer.analyze(
            pNewAudioModel->getAudioBuffer().getArrayOfReadPointers(),
            pNewAudioModel->getNumChannels(),
            pNewAudioModel->getLengthInSamples(),
            pNewAudioModel->getSampleRate());

        {
          juce::ScopedLock lock(dataLock);
          pAudioModel = std::move(pNewAudioModel);
          analysisResult = std::move(newAnalysisResult);
        }
      } catch (const std::exception &e) {
        juce::Logger::writeToLog("File analysis error: " +
                                 juce::String(e.what()));
      }

      isAnalysisRunning = false;
    }).detach();
  });
}

void MainComponent::setMenuBarBounds() {
  auto area = getLocalBounds();

  pMenuBarComponent->setBounds(
      area.removeFromTop(getLookAndFeel().getDefaultMenuBarHeight()));

  imguiOffsetY = getLookAndFeel().getDefaultMenuBarHeight();
}

void MainComponent::newOpenGLContextCreated() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGui_ImplJuce_Init(*this, glctx);
  ImGui_ImplOpenGL3_Init();
};

void MainComponent::renderOpenGL() {

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplJuce_NewFrame();
  ImGui::NewFrame();

  if (isAnalysisRunning) {
    ImGui::SetNextWindowPos(ImVec2(getWidth() * 0.5f, getHeight() * 0.5f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::Begin("Status", nullptr,
                 ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("%s", statusMessage.c_str());
    ImGui::End();
  }

  auto commonFlags =
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavFocus;

  {
    juce::ScopedLock lock(dataLock);

    ImGui::SetNextWindowPos(ImVec2(0, 0 + imguiOffsetY));
    ImGui::SetNextWindowSize(ImVec2(0.7 * getWidth(), 0.3 * getHeight()));

    ImGui::Begin("Audio waveform", NULL, commonFlags);

    waveformPanel.render(pAudioModel.get(), getWidth(), getHeight());

    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(0.7 * getWidth(), 0 + imguiOffsetY));
    ImGui::SetNextWindowSize(ImVec2(0.3 * getWidth(), 0.3 * getHeight()));

    ImGui::Begin("Headers", NULL, commonFlags);

    headerPanel.render(pAudioModel.get());

    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(0, 0.35 * getHeight()));
    ImGui::SetNextWindowSize(ImVec2(0.7 * getWidth(), 0.3 * getHeight()));

    ImGui::Begin("Frame parameters", NULL, commonFlags);

    std::vector<std::pair<std::string, ParameterType>> chosenParameters;

    for (auto frameParameterNameTypePair : frameParameters) {
      if (chosenFrameParameters[frameParameterNameTypePair]) {
        chosenParameters.push_back(frameParameterNameTypePair);
      }
    }

    frameParametersPanel.render(pAudioModel.get(), analysisResult, getWidth(),
                                getHeight(), chosenParameters);

    ImGui::End();
  }

  ImGui::SetNextWindowPos(ImVec2(0.7 * getWidth(), 0.35 * getHeight()));
  ImGui::SetNextWindowSize(ImVec2(0.3 * getWidth(), 0.3 * getHeight()));

  ImGui::Begin("Parameters selection");

  for (auto frameParameterNameTypePair : frameParameters) {
    ImGui::Checkbox(frameParameterNameTypePair.first.c_str(),
                    &chosenFrameParameters[frameParameterNameTypePair]);
  }

  ImGui::End();

  ImGui::Render();

  // background begin
  juce::gl::glClearColor(0, 0, 0, 1);
  juce::gl::glClear(juce::gl::GL_COLOR_BUFFER_BIT);
  // background end

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
};

void MainComponent::openGLContextClosing() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplJuce_Shutdown();
  ImGui::DestroyContext();
  ImPlot::DestroyContext();
};
