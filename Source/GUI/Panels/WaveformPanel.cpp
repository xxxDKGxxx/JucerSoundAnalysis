#include "WaveformPanel.h"
#include "imgui.h"
#include "implot.h"

void WaveformPanel::render(const AudioModel *pAudioModel, int width,
                           int height) {
  if (pAudioModel != nullptr && pAudioModel->getLengthInSamples() > 0 &&
      ImPlot::BeginPlot("Audio Waveform", ImVec2(0.7 * width, 0.25 * height))) {
    ImPlot::PlotLine("Signal", pAudioModel->getAudioBuffer().getReadPointer(0),
                     pAudioModel->getLengthInSamples(),
                     1.0 / pAudioModel->getSampleRate(), 0);
    ImPlot::EndPlot();
  }
}
