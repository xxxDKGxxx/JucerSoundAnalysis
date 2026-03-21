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
  static void plotFramesBool(const std::string& parameterName,
                             const std::vector<FrameResult> &frames,
                             double sampleRate,
                             const std::vector<bool>& boolValues);
};
