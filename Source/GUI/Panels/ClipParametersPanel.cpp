#include "ClipParametersPanel.h"

#include "implot.h"

#include <string>

namespace {

void plotClipBool(const std::string &parameterName,
                   const std::vector<bool> &boolValues, double xScale,
                   double xStart, double clipWindowDuration) {
  if (boolValues.empty())
    return;

  auto yMin = ImPlot::GetPlotLimits().Y.Min;
  auto yMax = ImPlot::GetPlotLimits().Y.Max;

  for (size_t i = 0; i < boolValues.size(); ++i) {
    if (!boolValues[i]) {
      continue;
    }

    double tCenter = i * xScale + xStart;
    double tStart = tCenter - clipWindowDuration / 2.0;
    double tEnd = tCenter + clipWindowDuration / 2.0;

    double xs[2] = {std::max(0.0, tStart), tEnd};
    double ys1[2] = {yMin, yMin};
    double ys2[2] = {yMax, yMax};

    ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.15f);
    ImPlot::PlotShaded(parameterName.c_str(), xs, ys1, ys2, 2);
  }
}

} // namespace

void ClipParametersPanel::render(
    const AudioModel *pAudioModel, const AnalysisResult &analysisResult,
    int width, int height, const std::vector<std::string> &chosenParameters) {
  if (pAudioModel == nullptr || analysisResult.channels.empty()) {
    ImGui::TextDisabled("No audio loaded.");
    return;
  }

  const auto &channelResult = analysisResult.channels[0];
  const int clipWindowSecondsDisplay =
      static_cast<int>(analysisResult.clipWindowSeconds + 0.5);
  const std::string plotTitle =
      "Clip-level timeline (" + std::to_string(clipWindowSecondsDisplay) + "s window)";

  if (ImPlot::BeginPlot(plotTitle.c_str(),
                        ImVec2(0.7f * width, 0.28f * height))) {
    ImPlot::SetupAxisLimits(ImAxis_X1, 0, pAudioModel->getLengthInSeconds(),
                            ImGuiCond_FirstUseEver);

    const double sampleRate = pAudioModel->getSampleRate();
    const double xScale =
        static_cast<double>(analysisResult.hopSize * analysisResult.clipWindowFrames) /
        sampleRate;
    const double xStart =
        (static_cast<double>(analysisResult.frameSize) / 2.0 +
         static_cast<double>(analysisResult.hopSize) *
             (static_cast<double>(analysisResult.clipWindowFrames) - 1.0) / 2.0) /
        sampleRate;
    
    const double clipWindowDuration = 
        static_cast<double>(analysisResult.hopSize * analysisResult.clipWindowFrames) / sampleRate;

    // First pass: Draw shaded background for boolean parameters
    for (const auto &parameterName : chosenParameters) {
      const auto itBool =
          channelResult.clipTimeSeriesBoolParameters.find(parameterName);
      if (itBool != channelResult.clipTimeSeriesBoolParameters.end() &&
          !itBool->second.empty()) {
        plotClipBool(parameterName, itBool->second, xScale, xStart,
                     clipWindowDuration);
      }
    }

    // Second pass: Draw lines for float parameters (on top)
    for (const auto &parameterName : chosenParameters) {
      const auto itFloat =
          channelResult.clipTimeSeriesFloatParameters.find(parameterName);
      if (itFloat != channelResult.clipTimeSeriesFloatParameters.end() &&
          !itFloat->second.empty()) {
        ImPlot::PlotLine(parameterName.c_str(), itFloat->second.data(),
                         static_cast<int>(itFloat->second.size()), xScale,
                         xStart);
      }
    }

    ImPlot::EndPlot();
  }

  if (chosenParameters.empty()) {
    ImGui::TextDisabled("Select clip-level parameters to plot.");
  }
}
