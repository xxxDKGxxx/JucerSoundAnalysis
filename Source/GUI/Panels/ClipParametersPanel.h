#pragma once

#include "../../Analysis/AudioAnalyzer.h"
#include "../Model/AudioModel.h"
#include <imgui.h>
#include <string>
#include <vector>

class ClipParametersPanel {
public:
  static void render(const AudioModel *pAudioModel,
                     const AnalysisResult &analysisResult, int width,
                     int height,
                     const std::vector<std::string> &chosenParameters);
};
