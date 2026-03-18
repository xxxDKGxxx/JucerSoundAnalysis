#pragma once

#include "../../Analysis/AudioAnalyzer.h"
#include "../Model/AudioModel.h"
#include <string>
#include <utility>
#include <vector>

enum ParameterType {
  Float,
  Bool,
};

class FrameParametersPanel {
public:
  static void
  render(const AudioModel *pAudioModel, AnalysisResult &analysisResult,
         int width, int height,
         std::vector<std::pair<std::string, ParameterType>> &chosenParameters);

private:
  static void plotFramesFloat(std::string parameterName,
                              std::vector<FrameResult> &frames,
                              std::vector<float> &frameMiddles);
  static void plotFramesBool(std::string parameterName,
                             std::vector<FrameResult> &frames);
};
