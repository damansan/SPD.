#include "MovementMonitorWindow.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>

#include "Engine/ECS/EntityFinder.h"
#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Components/StateMachineComponent.h"

#include "Game/ECS/Components/PlayerLocomotionComponent.h"

namespace
{
	constexpr ImU32 kBackColor =       IM_COL32(25, 25, 30, 255);
	constexpr ImU32 kGridColor =       IM_COL32(60, 60, 70, 255);
	constexpr ImU32 kZeroColor =       IM_COL32(130, 130, 140, 255);
	constexpr ImU32 kLabelColor =      IM_COL32(150, 150, 160, 255);
	constexpr ImU32 kHorizontalColor = IM_COL32(66, 150, 250, 255);  // 水平の速さ(青)
	constexpr ImU32 kDesiredColor =    IM_COL32(250, 200, 60, 220);  // 目標の速さ(黄)
	constexpr ImU32 kVerticalColor =   IM_COL32(120, 220, 140, 255); // 縦の速さ(緑)

	constexpr float kBandHeight = 16.0f; // ステートの帯の高さ

	// ステートの色(初めて出てきた順に使う)
	constexpr ImU32 kStateColors[] =
	{
		IM_COL32(90, 110, 140, 255),
		IM_COL32(70, 140, 100, 255),
		IM_COL32(160, 110, 60, 255),
		IM_COL32(130, 80, 150, 255),
		IM_COL32(150, 70, 80, 255),
		IM_COL32(120, 120, 60, 255),
	};
	constexpr int kStateColorCount = sizeof(kStateColors) / sizeof(kStateColors[0]);
}

// 値を1つ記録する
void MovementMonitorWindow::Record(entt::registry& registry, float elapsedTime)
{
	if (paused) return;

	// カテゴリが Player のエンティティを記録する
	entt::entity player = FindFirstByCategory(registry, EntityCategory::Player);
	if (player == entt::null) return;

	const auto* movement = registry.try_get<MovementComponent>(player);
	if (!movement) return;
	const auto* stateMachine = registry.try_get<StateMachineComponent>(player);
	const auto* locomotion = registry.try_get<PlayerLocomotionComponent>(player);

	totalTime += elapsedTime;

	Sample s;
	s.time = totalTime;
	s.horizontalSpeed = sqrtf(movement->velocity.x * movement->velocity.x + movement->velocity.z * movement->velocity.z);
	s.desiredSpeed = locomotion ? locomotion->desiredSpeed : 0.0f;
	s.verticalSpeed = movement->velocity.y;
	s.stateIndex = FindOrAddState(stateMachine ? stateMachine->currentStateName : "(no state)");
	samples.push_back(s);

	// 古い記録を捨てる
	while (!samples.empty() && samples.front().time < totalTime - kKeepSeconds)
	{
		samples.pop_front();
	}
}

// 記録をグラフに描く
void MovementMonitorWindow::Draw()
{
	if (!isOpen) return;

	if (!ImGui::Begin("Movement Monitor", &isOpen))
	{
		ImGui::End();
		return;
	}

	ImGui::Checkbox("Stop", &paused);

	if (samples.size() < 2)
	{
		ImGui::TextDisabled("No data");
		ImGui::End();
		return;
	}

	// 表示範囲の最大値に合わせて、縦の目盛りを決める
	const float startTime = samples.back().time - kViewSeconds;
	float maxHorizontal = 6.0f;
	float maxVertical = 10.0f;
	for (const Sample& s : samples)
	{
		if (s.time < startTime) continue;
		maxHorizontal = (std::max)(maxHorizontal, (std::max)(s.horizontalSpeed, s.desiredSpeed));
		maxVertical = (std::max)(maxVertical, fabsf(s.verticalSpeed));
	}

	ImGui::TextUnformatted("Horizontal (m/s)");
	const Series horizontal[] =
	{
		{ &Sample::desiredSpeed,    kDesiredColor,    1.5f },
		{ &Sample::horizontalSpeed, kHorizontalColor, 2.0f },
	};
	DrawGraph(180.0f, 0.0f, maxHorizontal * 1.1f, 1.0f, horizontal, 2);

	ImGui::TextUnformatted("Vertical (m/s)");
	const Series vertical[] =
	{
		{ &Sample::verticalSpeed, kVerticalColor, 2.0f },
	};
	DrawGraph(140.0f, -maxVertical * 1.1f, maxVertical * 1.1f, 5.0f, vertical, 1);

	ImGui::End();
}

// グラフを1つ描く
void MovementMonitorWindow::DrawGraph(float height, float minValue, float maxValue, float gridStep, const Series* series, int seriesCount)
{
	ImDrawList* dl = ImGui::GetWindowDrawList();

	const ImVec2 pos = ImGui::GetCursorScreenPos();
	const float width = ImGui::GetContentRegionAvail().x;
	const float plotHeight = height - kBandHeight;

	const ImVec2 plotMin = pos;
	const ImVec2 plotMax = ImVec2(pos.x + width, pos.y + plotHeight);
	const ImVec2 bandMax = ImVec2(pos.x + width, pos.y + height);

	// 時間 → X座標、値 → Y座標
	const float endTime = samples.back().time;
	const float startTime = endTime - kViewSeconds;
	auto ToX = [&](float t) { return plotMin.x + (t - startTime) / kViewSeconds * width; };
	auto ToY = [&](float v)
		{
			float rate = std::clamp((v - minValue) / (maxValue - minValue), 0.0f, 1.0f);
			return plotMax.y - rate * plotHeight;
		};

	dl->PushClipRect(plotMin, bandMax, true);

	// 背景
	dl->AddRectFilled(plotMin, plotMax, kBackColor);

	// 横の目盛り
	for (float v = ceilf(minValue / gridStep) * gridStep; v <= maxValue; v += gridStep)
	{
		float y = ToY(v);
		bool isZero = fabsf(v) < 0.001f;
		dl->AddLine(ImVec2(plotMin.x, y), ImVec2(plotMax.x, y), isZero ? kZeroColor : kGridColor);

		char label[16];
		snprintf(label, sizeof(label), "%.0f", v);
		dl->AddText(ImVec2(plotMin.x + 2.0f, y - 14.0f), kLabelColor, label);
	}

	// 縦の目盛り
	for (float t = ceilf(startTime); t <= endTime; t += 1.0f)
	{
		float x = ToX(t);
		dl->AddLine(ImVec2(x, plotMin.y), ImVec2(x, plotMax.y), kGridColor);
	}

	// 値の線
	for (int k = 0; k < seriesCount; ++k)
	{
		const Series& s = series[k];
		for (size_t i = 1; i < samples.size(); ++i)
		{
			const Sample& a = samples[i - 1];
			const Sample& b = samples[i];
			if (b.time < startTime) continue;

			dl->AddLine(ImVec2(ToX(a.time), ToY(a.*(s.value))), ImVec2(ToX(b.time), ToY(b.*(s.value))), s.color, s.thickness);
		}
	}

	// ステートの帯
	size_t runStart = 0;
	for (size_t i = 1; i <= samples.size(); ++i)
	{
		bool runEnds = (i == samples.size()) || (samples[i].stateIndex != samples[runStart].stateIndex);
		if (!runEnds) continue;

		const Sample& first = samples[runStart];
		const Sample& end = samples[i - 1];
		float x0 = ToX(first.time);
		float x1 = ToX(end.time);

		if (end.time >= startTime)
		{
			ImU32 color = kStateColors[first.stateIndex % kStateColorCount];
			dl->AddRectFilled(ImVec2(x0, plotMax.y), ImVec2(x1, bandMax.y), color);

			// 区間に収まる時だけ、ステート名を書く
			const char* name = stateNames[first.stateIndex].c_str();
			float textWidth = ImGui::CalcTextSize(name).x;
			float textX = (std::max)(x0, plotMin.x) + 3.0f;
			if (textX + textWidth < x1)
			{
				dl->AddText(ImVec2(textX, plotMax.y + 1.0f), IM_COL32_WHITE, name);
			}
		}

		runStart = i;
	}

	dl->PopClipRect();

	// 描いた分の場所を確保する
	ImGui::Dummy(ImVec2(width, height));
}

// ステート名を番号に変える
int MovementMonitorWindow::FindOrAddState(const std::string& name)
{
	for (int i = 0; i < (int)stateNames.size(); ++i)
	{
		if (stateNames[i] == name) return i;
	}
	stateNames.push_back(name);
	return (int)stateNames.size() - 1;
}