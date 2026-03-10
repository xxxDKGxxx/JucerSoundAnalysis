#include "MainComponent.h"
#include "imgui.h"
#include "implot.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_audio_formats/juce_audio_formats.h"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <cstddef>
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

  glctx.setOpenGLVersionRequired(juce::OpenGLContext::openGL3_2);
  glctx.setRenderer(this);
  glctx.attachTo(*this);
  glctx.setContinuousRepainting(true);
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

  auto commonFlags =
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavFocus;

  ImGui::SetNextWindowPos(ImVec2(0, 0 + imguiOffsetY));
  ImGui::SetNextWindowSize(ImVec2(0.7 * getWidth(), 0.3 * getHeight()));

  ImGui::Begin("Audio waveform", NULL, commonFlags);

  if (pAudioModel != nullptr && pAudioModel->getLengthInSamples() > 0 &&
      ImPlot::BeginPlot("Przebieg Audio",
                        ImVec2(0.7 * getWidth(), 0.25 * getHeight()))) {
    ImPlot::PlotLine("", pAudioModel->getAudioBuffer().getReadPointer(0),
                     pAudioModel->getLengthInSamples());
    ImPlot::EndPlot();
  }

  ImGui::End();

  ImGui::SetNextWindowPos(ImVec2(0.7 * getWidth(), 0 + imguiOffsetY));
  ImGui::SetNextWindowSize(ImVec2(0.3 * getWidth(), 0.3 * getHeight()));

  ImGui::Begin("Headers", NULL, commonFlags);
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
