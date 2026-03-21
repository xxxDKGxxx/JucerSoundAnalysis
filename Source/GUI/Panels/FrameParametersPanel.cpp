#include "FrameParametersPanel.h"
#include "implot.h"
#include <vector>

void FrameParametersPanel::render(
    const AudioModel *pAudioModel, AnalysisResult &analysisResult, int width,
    int height,
    std::vector<std::pair<std::string, ParameterType>> &chosenParameters) {
  if (pAudioModel != nullptr && analysisResult.channels.size() > 0) {
    auto &frames = analysisResult.channels[0].frames;

    std::vector<float> frameMiddles;
    std::vector<float> zcr;
    std::vector<float> timeAxis;

    for (auto i = 0; i < pAudioModel->getLengthInSamples(); i++) {
      timeAxis.push_back(i / pAudioModel->getSampleRate());
    }

    frameMiddles.resize((frames.size()));
    zcr.resize(frames.size());

    std::transform(frames.begin(), frames.end(), frameMiddles.begin(),
                   [&pAudioModel](FrameResult frameResult) {
                     return (frameResult.startSample + frameResult.endSample) /
                            2.0 / pAudioModel->getSampleRate();
                   });

    if (ImPlot::BeginPlot("Frame parameters",
                          ImVec2(0.7 * width, 0.25 * height))) {
      ImPlot::PlotLine("", timeAxis.data(),
                       pAudioModel->getAudioBuffer().getReadPointer(0),
                       pAudioModel->getLengthInSamples());

      for (auto parameterNameTypePair : chosenParameters) {
        auto parameterName = parameterNameTypePair.first;
        auto parameterType = parameterNameTypePair.second;

        switch (parameterType) {
        case Float:
          plotFramesFloat(parameterName, frames, frameMiddles);
          break;
        case Bool:
          plotFramesBool(parameterName, frames, pAudioModel->getSampleRate());
          break;
        default:
          break;
        }
      }

      ImPlot::EndPlot();
    }
  }
}
void FrameParametersPanel::plotFramesFloat(std::string parameterName,
                                           std::vector<FrameResult> &frames,
                                           std::vector<float> &frameMiddles) {
  std::vector<float> parameterValues;

  parameterValues.resize(frames.size());

  std::transform(frames.begin(), frames.end(), parameterValues.begin(),
                 [&parameterName](FrameResult frameResult) {
                   return frameResult.getDouble(parameterName);
                 });

  ImPlot::PlotLine(parameterName.c_str(), frameMiddles.data(),
                   parameterValues.data(), frameMiddles.size());
}

void FrameParametersPanel::plotFramesBool(std::string parameterName,
                                          std::vector<FrameResult> &frames,
                                          double sampleRate) {
  auto yMin = ImPlot::GetPlotLimits().Y.Min;
  auto yMax = ImPlot::GetPlotLimits().Y.Max;

  for (auto frameResult : frames) {
    if (!frameResult.getBool(parameterName)) {
      continue;
    }

    double xs[2] = {(double)frameResult.startSample / sampleRate,
                    (double)frameResult.endSample / sampleRate};

    double ys1[2] = {yMin, yMin};
    double ys2[2] = {yMax, yMax};

    ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.3);

    ImPlot::PlotShaded(parameterName.c_str(), xs, ys1, ys2, 2);
  }
}
