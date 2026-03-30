#include "ClipParametersPanel.h"

#include "implot.h"

#include <string>

void ClipParametersPanel::render(
		const AudioModel *pAudioModel, const AnalysisResult &analysisResult,
		int width, int height, const std::vector<std::string> &chosenParameters) {
	if (pAudioModel == nullptr || analysisResult.channels.empty()) {
		ImGui::TextDisabled("No audio loaded.");
		return;
	}

	const auto &channelResult = analysisResult.channels[0];
	const int clipWindowSeconds =
				static_cast<int>(analysisResult.clipWindowSeconds + 0.5);
	const std::string plotTitle =
				"Clip-level timeline (" + std::to_string(clipWindowSeconds) + "s window)";

	if (ImPlot::BeginPlot(plotTitle.c_str(),
												ImVec2(0.7f * width, 0.28f * height))) {
		ImPlot::SetupAxisLimits(ImAxis_X1, 0, pAudioModel->getLengthInSeconds(),
														ImGuiCond_FirstUseEver);

		const double xScale =
				static_cast<double>(analysisResult.hopSize) / pAudioModel->getSampleRate();
		const double xStart = static_cast<double>(analysisResult.frameSize) / 2.0 /
													pAudioModel->getSampleRate();

		for (const auto &parameterName : chosenParameters) {
			const auto it =
					channelResult.clipTimeSeriesFloatParameters.find(parameterName);
			if (it != channelResult.clipTimeSeriesFloatParameters.end() &&
					!it->second.empty()) {
				ImPlot::PlotLine(parameterName.c_str(), it->second.data(),
												 static_cast<int>(it->second.size()), xScale, xStart);
			}
		}

		ImPlot::EndPlot();
	}

	if (chosenParameters.empty()) {
		ImGui::TextDisabled("Select clip-level parameters to plot.");
	}
}
