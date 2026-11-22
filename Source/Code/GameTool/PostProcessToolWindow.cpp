#include "PostProcessToolWindow.h"

#include "Engine/Rendering/PostProcess.h"

#include <imgui.h>
#include <string>

// 調整画面を描く
void PostProcessToolWindow::Draw(PostProcessSettings& settings)
{
	if (!isOpen) return;

	if (!ImGui::Begin("Post Process", &isOpen))
	{
		ImGui::End();
		return;
	}

	// トーンマップ
	ImGui::SeparatorText("Tone Mapping");
	ImGui::SliderFloat  ("Exposure", &settings.exposure, 0.0f, 4.0f, "%.2f");
	ImGui::Checkbox     ("ACES", &settings.tonemapEnabled);

	// カラー調整
	ImGui::SeparatorText("Color Adjustments");
	ImGui::SliderFloat  ("Contrast", &settings.contrast, 0.0f, 2.0f, "%.2f");
	ImGui::ColorEdit3   ("Color Filter", &settings.colorFilter.x);
	ImGui::SliderFloat  ("Saturation", &settings.saturation, 0.0f, 2.0f, "%.2f");

	// ビネット
	ImGui::SeparatorText("Vignette");
	ImGui::SliderFloat  ("Intensity", &settings.vignetteIntensity, 0.0f, 1.0f, "%.2f");
	ImGui::SliderFloat  ("Smoothness", &settings.vignetteSmoothness, 0.01f, 1.0f, "%.2f");
	ImGui::ColorEdit3   ("Vignette Color", &settings.vignetteColor.x);

	// ブルーム
	ImGui::SeparatorText("Bloom");
	ImGui::SliderFloat("Threshold", &settings.bloomThreshold, 0.0f, 5.0f);
	ImGui::SliderFloat("Intensity##Bloom", &settings.bloomIntensity, 0.0f, 3.0f);

	// ラジアルブラー
	ImGui::SeparatorText("Radial Blur");
	ImGui::Checkbox("Enabled##RadialBlur", &settings.radialBlurEnabled);
	// OFF の間は触れないようにする
	ImGui::BeginDisabled(!settings.radialBlurEnabled);
	ImGui::SliderFloat("Strength##RadialBlur", &settings.radialBlurStrength, 0.0f, 0.5f, "%.2f");
	ImGui::SliderFloat("Radius##RadialBlur", &settings.radialBlurRadius, 0.0f, 2.0f, "%.2f");
	ImGui::SliderFloat("Softness##RadialBlur", &settings.radialBlurSoftness, 0.01f, 2.0f, "%.2f");
	ImGui::EndDisabled();

	// デバッグ表示
	ImGui::SeparatorText("Debug");
	const char* debugViews[] = { "None", "Bloom Extract", "Bloom Blur", "Bloom Result" };
	int debugView = static_cast<int>(settings.debugView);
	if (ImGui::Combo("View", &debugView, debugViews, IM_ARRAYSIZE(debugViews)))
	{
		settings.debugView = static_cast<PostProcessDebugView>(debugView);
	}

	// ぼかした段を見るときだけ、どの段かを選ぶ
	if (settings.debugView == PostProcessDebugView::BloomBlur)
	{
		ImGui::SliderInt("Level", &settings.debugBloomLevel, 0, PostProcess::kBloomLevels - 1);
		ImGui::TextDisabled("0 = 1/2 ... %d = 1/%d", PostProcess::kBloomLevels - 1, 1 << PostProcess::kBloomLevels);
	}

	ImGui::Spacing();
	ImGui::Separator();

	// 初期値に戻す
	if (ImGui::Button("Reset"))
	{
		settings = PostProcessSettings{};
	}
	ImGui::SameLine();

	ImGui::End();
}
