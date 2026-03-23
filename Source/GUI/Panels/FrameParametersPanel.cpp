#include "FrameParametersPanel.h"
#include "implot.h"
#include <optional>
#include <vector>

void FrameParametersPanel::render(
    const AudioModel *pAudioModel, AnalysisResult &analysisResult, int width,
    int height,
    std::vector<std::pair<std::string, ParameterType>> &chosenParameters) {
  if (pAudioModel != nullptr && !analysisResult.channels.empty()) {
    const auto &channelResult = analysisResult.channels[0];
    const auto &frames = channelResult.frames;

    if (ImPlot::BeginPlot("Frame parameters",
                          ImVec2(0.7 * width, 0.25 * height))) {
      // Plot waveform as background (optimized)
      ImPlot::PlotLine("Waveform",
                       pAudioModel->getAudioBuffer().getReadPointer(0),
                       pAudioModel->getLengthInSamples(),
                       1.0 / pAudioModel->getSampleRate(), 0);

      auto xscale =
          (double)analysisResult.hopSize / pAudioModel->getSampleRate();
      auto xstart =
          (double)analysisResult.frameSize / 2.0 / pAudioModel->getSampleRate();

      for (auto parameterNameTypePair : chosenParameters) {
        auto parameterName = parameterNameTypePair.first;
        auto parameterType = parameterNameTypePair.second;

        if (parameterType == Float) {
          auto it =
              channelResult.precomputedFloatParameters.find(parameterName);
          if (it != channelResult.precomputedFloatParameters.end()) {
            ImPlot::PlotLine(parameterName.c_str(), it->second.data(),
                             static_cast<int>(it->second.size()), xscale,
                             xstart);
          }
        } else if (parameterType == Bool) {
          auto it = channelResult.precomputedBoolParameters.find(parameterName);
          if (it != channelResult.precomputedBoolParameters.end()) {
            plotFramesBool(parameterName, frames, pAudioModel->getSampleRate(),
                           it->second);
          }
        } else if (parameterType == InterpolatedFloatOption) {
          auto it = channelResult.precomputedOptionalFloatParameters.find(
              parameterName);
          plotFramesInterpolatedFloatOption(parameterName, it->second, xscale,
                                            xstart);
        }
      }

      ImPlot::EndPlot();
    }
  }
}

void FrameParametersPanel::plotFramesBool(
    const std::string &parameterName, const std::vector<FrameResult> &frames,
    double sampleRate, const std::vector<bool> &boolValues) {
  auto yMin = ImPlot::GetPlotLimits().Y.Min;
  auto yMax = ImPlot::GetPlotLimits().Y.Max;

  for (size_t i = 0; i < frames.size(); ++i) {
    if (!boolValues[i]) {
      continue;
    }

    const auto &frameResult = frames[i];
    double xs[2] = {(double)frameResult.startSample / sampleRate,
                    (double)frameResult.endSample / sampleRate};

    double ys1[2] = {yMin, yMin};
    double ys2[2] = {yMax, yMax};

    ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.3);
    ImPlot::PlotShaded(parameterName.c_str(), xs, ys1, ys2, 2);
  }
}

void FrameParametersPanel::plotFramesInterpolatedFloatOption(
    const std::string &parameterName,
    const std::vector<std::optional<double>> &floatOptionValues, double xScale,
    double xStart) {
  std::vector<double> xs;
  std::vector<double> ys;

  xs.reserve(floatOptionValues.size());
  ys.reserve(floatOptionValues.size());

  for (auto i = 0; i < floatOptionValues.size(); i++) {
    if (floatOptionValues[i].has_value()) {
      xs.push_back(i * xScale + xStart);
      ys.push_back(floatOptionValues[i].value());
    }
  }

  if (xs.empty())
    return;

  ImPlot::PlotLine(parameterName.c_str(), xs.data(), ys.data(), xs.size());
}
