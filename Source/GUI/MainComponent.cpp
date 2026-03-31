#include "MainComponent.h"
#include "Panels/FrameParametersPanel.h"
#include "imgui.h"
#include "implot.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_audio_formats/juce_audio_formats.h"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

//==============================================================================
MainComponent::MainComponent() {
  setSize(1200, 800);

  menuModel.onLoadSoundFileClick = [this]() { this->loadWavFile(); };
  menuModel.onExportParametersClick = [this]() {
    this->exportParametersToTxt();
  };

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
  frameParameters.push_back(std::pair("crestFactor(da)", Float));
  frameParameters.push_back(std::pair("isSilent", Bool));
  frameParameters.push_back(std::pair("isVoiced", Bool));
  frameParameters.push_back(
      std::pair("AutocorrelationF0(kHz)", InterpolatedFloatOption));
  frameParameters.push_back(std::pair("AMDFF0(kHz)", InterpolatedFloatOption));

  clipParameters.push_back("VSTD");
  clipParameters.push_back("VDR");
  clipParameters.push_back("VU");
  clipParameters.push_back("ZSTD");
  clipParameters.push_back("LSTER");
  clipParameters.push_back("EnergyEntropy");
  clipParameters.push_back("HZCRR");

  for (auto frameParameterPair : frameParameters) {
    chosenFrameParameters[frameParameterPair] = false;
  }

  for (const auto &clipParameter : clipParameters) {
    chosenClipParameters[clipParameter] = false;
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

    if (file == juce::File{}) {
      return;
    }

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
        auto newAnalysisResult = analyzeAudio(*pNewAudioModel);

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

      delete audioFormatReader;
    }).detach();
  });
}

void MainComponent::exportParametersToTxt() {
  AnalysisResult resultToExport;
  const AudioModel *audioModel = nullptr;
  std::vector<std::string> selectedFrameParameterNames;
  std::vector<std::string> selectedClipParameterNames;

  {
    juce::ScopedLock lock(dataLock);
    audioModel = pAudioModel.get();
    if (audioModel == nullptr || analysisResult.channels.empty()) {
      juce::Logger::writeToLog("Export canceled: no analyzed audio available.");
      return;
    }

    resultToExport = analysisResult;

    for (const auto &frameParameter : frameParameters) {
      if (chosenFrameParameters[frameParameter]) {
        selectedFrameParameterNames.push_back(frameParameter.first);
      }
    }

    for (const auto &clipParameter : clipParameters) {
      if (chosenClipParameters[clipParameter]) {
        selectedClipParameterNames.push_back(clipParameter);
      }
    }
  }

  const auto isFrameParameterSelected =
      [&selectedFrameParameterNames](const std::string &name) {
        return std::find(selectedFrameParameterNames.begin(),
                         selectedFrameParameterNames.end(),
                         name) != selectedFrameParameterNames.end();
      };

  const auto isClipParameterSelected =
      [&selectedClipParameterNames](const std::string &name) {
        return std::find(selectedClipParameterNames.begin(),
                         selectedClipParameterNames.end(),
                         name) != selectedClipParameterNames.end();
      };

  std::ostringstream oss;
  oss << std::fixed << std::setprecision(6);
  oss << "# Sound Analyzer - parameters export\n";
  oss << "sampleRate=" << audioModel->getSampleRate() << "\n";
  oss << "bitsPerSample=" << audioModel->getBitsPerSample() << "\n";
  oss << "numChannels=" << audioModel->getNumChannels() << "\n";
  oss << "lengthSamples=" << audioModel->getLengthInSamples() << "\n";
  oss << "lengthSeconds=" << audioModel->getLengthInSeconds() << "\n";
  oss << "frameSize=" << resultToExport.frameSize << "\n";
  oss << "hopSize=" << resultToExport.hopSize << "\n";
  oss << "clipWindowSeconds=" << resultToExport.clipWindowSeconds << "\n\n";
  oss << "selectedFrameParameters=" << selectedFrameParameterNames.size()
      << "\n";
  for (const auto &name : selectedFrameParameterNames) {
    oss << "frame=" << name << "\n";
  }
  oss << "selectedClipParameters=" << selectedClipParameterNames.size() << "\n";
  for (const auto &name : selectedClipParameterNames) {
    oss << "clip=" << name << "\n";
  }
  oss << "\n";

  const double frameStepSeconds =
      (resultToExport.sampleRate > 0.0)
          ? (static_cast<double>(resultToExport.hopSize) /
             resultToExport.sampleRate)
          : 0.0;

  for (const auto &channel : resultToExport.channels) {
    oss << "[channel " << channel.channelIndex << "]\n";

    oss << "clipFloatParameters\n";
    for (const auto &[name, value] : channel.clipFloatParameters) {
      if (!isClipParameterSelected(name))
        continue;
      oss << name << "=" << value << "\n";
    }
    oss << "\n";

    oss << "clipTimeSeriesFloatParameters\n";
    for (const auto &[name, values] : channel.clipTimeSeriesFloatParameters) {
      if (!isClipParameterSelected(name))
        continue;

      for (size_t i = 0; i < values.size(); ++i) {
        oss << name << ";index=" << i << ";time=" << (i * frameStepSeconds)
            << ";value=" << values[i] << "\n";
      }
    }
    oss << "\n";

    oss << "frameFloatParameters\n";
    for (const auto &[name, values] : channel.precomputedFloatParameters) {
      if (!isFrameParameterSelected(name))
        continue;

      for (size_t i = 0; i < values.size(); ++i) {
        oss << name << ";frame=" << i << ";time=" << (i * frameStepSeconds)
            << ";value=" << values[i] << "\n";
      }
    }
    oss << "\n";

    oss << "frameBoolParameters\n";
    for (const auto &[name, values] : channel.precomputedBoolParameters) {
      if (!isFrameParameterSelected(name))
        continue;

      for (size_t i = 0; i < values.size(); ++i) {
        oss << name << ";frame=" << i << ";time=" << (i * frameStepSeconds)
            << ";value=" << (values[i] ? "true" : "false") << "\n";
      }
    }
    oss << "\n";

    oss << "frameOptionalFloatParameters\n";
    for (const auto &[name, values] :
         channel.precomputedOptionalFloatParameters) {
      if (!isFrameParameterSelected(name))
        continue;

      for (size_t i = 0; i < values.size(); ++i) {
        oss << name << ";frame=" << i << ";time=" << (i * frameStepSeconds)
            << ";value=";
        if (values[i].has_value()) {
          oss << values[i].value();
        } else {
          oss << "null";
        }
        oss << "\n";
      }
    }

    oss << "\n";
  }

  pFileChooser.reset(new juce::FileChooser("Export parameters to TXT...",
                                           juce::File{}, "*.txt"));

  const auto chooserFlags = juce::FileBrowserComponent::saveMode |
                            juce::FileBrowserComponent::canSelectFiles |
                            juce::FileBrowserComponent::warnAboutOverwriting;

  const auto exportText = juce::String(oss.str());
  pFileChooser->launchAsync(
      chooserFlags, [text = exportText](const juce::FileChooser &fc) {
        auto target = fc.getResult();
        if (target == juce::File{})
          return;

        if (!target.hasFileExtension("txt")) {
          target = target.withFileExtension("txt");
        }

        const bool ok = target.replaceWithText(text);
        juce::Logger::writeToLog(
            ok ? ("Exported parameters to: " + target.getFullPathName())
               : ("Export failed: " + target.getFullPathName()));
      });
}

void MainComponent::reanalyzeCurrentAudio() {
  if (isAnalysisRunning)
    return;

  {
    juce::ScopedLock lock(dataLock);
    if (pAudioModel == nullptr)
      return;
  }

  isAnalysisRunning = true;
  statusMessage = "Re-analyzing...";

  std::thread([this]() {
    try {
      const AudioModel *pCurrentAudioModel = nullptr;

      {
        juce::ScopedLock lock(dataLock);
        if (pAudioModel == nullptr) {
          isAnalysisRunning = false;
          return;
        }

        pCurrentAudioModel = pAudioModel.get();
      }

      auto newAnalysisResult = analyzeAudio(*pCurrentAudioModel);

      {
        juce::ScopedLock lock(dataLock);
        analysisResult = std::move(newAnalysisResult);
      }
    } catch (const std::exception &e) {
      juce::Logger::writeToLog("Re-analysis error: " + juce::String(e.what()));
    }

    isAnalysisRunning = false;
  }).detach();
}

AnalysisResult MainComponent::analyzeAudio(const AudioModel &audioModel) const {
  AnalysisParams analysisParams;
  analysisParams.frameSize = static_cast<size_t>(selectedFrameSize);
  analysisParams.clipWindowSeconds =
      static_cast<double>(selectedClipWindowSeconds);

  return audioAnalyzer.analyze(
      audioModel.getAudioBuffer().getArrayOfReadPointers(),
      audioModel.getNumChannels(), audioModel.getLengthInSamples(),
      audioModel.getSampleRate(), analysisParams);
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

  ImGui::Text("Frame size");
  bool frameSizeChanged = false;
  frameSizeChanged |= ImGui::RadioButton("256", &selectedFrameSize, 256);
  ImGui::SameLine();
  frameSizeChanged |= ImGui::RadioButton("512", &selectedFrameSize, 512);
  ImGui::SameLine();
  frameSizeChanged |= ImGui::RadioButton("1024", &selectedFrameSize, 1024);

  if (frameSizeChanged) {
    reanalyzeCurrentAudio();
  }

  ImGui::Separator();

  for (auto frameParameterNameTypePair : frameParameters) {
    ImGui::Checkbox(frameParameterNameTypePair.first.c_str(),
                    &chosenFrameParameters[frameParameterNameTypePair]);
  }

  ImGui::End();

  ImGui::SetNextWindowPos(ImVec2(0, 0.68 * getHeight()));
  ImGui::SetNextWindowSize(ImVec2(0.7 * getWidth(), 0.30 * getHeight()));

  ImGui::Begin("Clip-level parameters", NULL, commonFlags);

  {
    juce::ScopedLock lock(dataLock);
    std::vector<std::string> chosenClipParameterNames;
    for (const auto &clipParameter : clipParameters) {
      if (chosenClipParameters[clipParameter]) {
        chosenClipParameterNames.push_back(clipParameter);
      }
    }

    clipParametersPanel.render(pAudioModel.get(), analysisResult, getWidth(),
                               getHeight(), chosenClipParameterNames);
  }

  ImGui::End();

  ImGui::SetNextWindowPos(ImVec2(0.7 * getWidth(), 0.68 * getHeight()));
  ImGui::SetNextWindowSize(ImVec2(0.3 * getWidth(), 0.30 * getHeight()));

  ImGui::Begin("Clip parameters selection", NULL, commonFlags);

  ImGui::Text("Clip window");
  bool clipWindowChanged = false;
  clipWindowChanged |= ImGui::RadioButton("1s", &selectedClipWindowSeconds, 1);
  ImGui::SameLine();
  clipWindowChanged |= ImGui::RadioButton("3s", &selectedClipWindowSeconds, 3);
  ImGui::SameLine();
  clipWindowChanged |= ImGui::RadioButton("5s", &selectedClipWindowSeconds, 5);

  if (clipWindowChanged) {
    reanalyzeCurrentAudio();
  }

  ImGui::Separator();

  for (const auto &clipParameter : clipParameters) {
    ImGui::Checkbox(clipParameter.c_str(),
                    &chosenClipParameters[clipParameter]);
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
