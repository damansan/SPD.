#include "StudioTimelineWidget.h"
#include "StudioContext.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
	const ImU32 RulerBgColor = IM_COL32(40, 42, 46, 255);
	const ImU32 TickColor = IM_COL32(120, 120, 120, 255);
	const ImU32 LabelColor = IM_COL32(200, 200, 200, 255);
	const ImU32 PinColor = IM_COL32(255, 82, 82, 255);
	const ImU32 RangeHandleColor = IM_COL32(255, 255, 255, 255);
	const ImU32 RangeHandleActiveColor = IM_COL32(255, 213, 79, 255);
}

// 目盛りを描いて、ピンのドラッグを処理する
bool StudioTimelineWidget::Begin(const char* id, float clipLength, float frameStep, float& inOutTime)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImVec2 cursor = ImGui::GetCursorScreenPos();
	const float availWidth = ImGui::GetContentRegionAvail().x;

	left = cursor.x + LeftPadding;
	width = (std::max)(availWidth - LeftPadding - RightPadding, 1.0f);
	top = cursor.y;
	length = (std::max)(clipLength, 0.0001f);

	// 目盛りの領域をつかめるようにする
	ImGui::InvisibleButton(id, ImVec2(availWidth, PinHeadHeight + RulerHeight));

	bool changed = false;
	if (ImGui::IsItemActive())
	{
		const float t = SnapToFrame(XToTime(ImGui::GetIO().MousePos.x), frameStep, length);
		if (t != inOutTime)
		{
			inOutTime = t;
			changed = true;
		}
	}
	time = inOutTime;

	DrawRuler(drawList, frameStep);
	return changed;
}

// ドラッグしたら animator を止めてポーズだけ更新する
bool StudioTimelineWidget::BeginWithAnimator(const char* id, float clipLength, float frameStep, AnimatorComponent& animator, StudioContext& context)
{
	if (!Begin(id, clipLength, frameStep, animator.animationTime)) return false;

	// スクラブ中は止めて、ポーズだけ更新する
	animator.isPlaying = false;
	animator.isEnd = false;
	context.requestPoseUpdate = true;
	return true;
}
// animator を再生するボタン
void StudioTimelineWidget::DrawAnimatorToolbar(AnimatorComponent& animator, float clipLength, float frameStep)
{
	if (!DrawToolbar(animator.isPlaying, animator.animationTime, frameStep)) return;

	if (animator.isPlaying && animator.animationTime >= clipLength)
	{
		animator.animationTime = 0.0f;
	}
	animator.isEnd = false;
}

// トラックを描き終えたら呼ぶ
void StudioTimelineWidget::End()
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const float bottom = ImGui::GetCursorScreenPos().y;
	const float x = TimeToX(time);

	// 細い線：頭の下から、トラックの下端まで
	drawList->AddLine(ImVec2(x, top + PinHeadHeight), ImVec2(x, bottom), PinColor, 1.0f);

	// 頭は最後に描いて、一番手前に出す
	DrawPinHead(drawList);
}
// 再生ボタンと現在時刻
bool StudioTimelineWidget::DrawToolbar(bool& isPlaying, float time, float frameStep)
{
	bool changed = false;

	if (ImGui::Button(isPlaying ? "■" : "▶", ImVec2(28.0f, 0.0f)))
	{
		isPlaying = !isPlaying;
		changed = true;
	}

	ImGui::SameLine();
	ImGui::Text("%.3f s / F%d", time, ToFrame(time, frameStep));

	return changed;
}

// フレームに吸い付かせる
float StudioTimelineWidget::SnapToFrame(float t, float frameStep, float clipLength)
{
	if (frameStep <= 0.0f) return std::clamp(t, 0.0f, clipLength);
	return std::clamp(std::round(t / frameStep) * frameStep, 0.0f, clipLength);
}
// 時刻からフレームの番号
int StudioTimelineWidget::ToFrame(float t, float frameStep)
{
	return (frameStep > 0.0f) ? (int)std::round(t / frameStep) : 0;
}

// 重ならない範囲の並びで、index 番目が動ける所
void StudioTimelineWidget::GetSpanLimits(const std::vector<TimeSpan>& spans, int index, float clipLength, float& outLower, float& outUpper)
{
	outLower = 0.0f;
	outUpper = clipLength;
	if (index < 0 || index >= (int)spans.size()) return;

	const float start = spans[index].start;
	const float end = spans[index].end;

	for (int i = 0; i < (int)spans.size(); i++)
	{
		if (i == index) continue;
		if (spans[i].end <= start + 0.0001f) outLower = (std::max)(outLower, spans[i].end);   // 前にある範囲
		if (spans[i].start >= end - 0.0001f) outUpper = (std::min)(outUpper, spans[i].start); // 後ろにある範囲
	}
}
// pinTime に新しい範囲を追加できるか
bool StudioTimelineWidget::FindAddEnd(const std::vector<TimeSpan>& spans, float pinTime, float clipLength, float minLength, float& outEnd)
{
	const float tolerance = minLength * 0.5f; // 小数の誤差として許す量

	float end = clipLength;
	for (const auto& s : spans)
	{
		if (s.end - s.start < tolerance) continue; // カットされて長さが無くなった範囲は、無いものとして扱う

		if (pinTime >= s.start - tolerance && pinTime < s.end - tolerance) return false; // 既存の範囲の中
		if (s.start >= pinTime - tolerance) end = (std::min)(end, s.start);              // 次の範囲の始まりまで
	}

	outEnd = end;
	return (end - pinTime) >= (minLength - tolerance);
}

// 範囲の端のピン
bool StudioTimelineWidget::DrawRangeHandles(const char* id, float handleY, float frameStep, float minLength, float& inOutStart, float& inOutEnd,
	float lowerLimit, float upperLimit, float lineTopY) 
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// 動ける範囲と、線の上端
	const float lower = lowerLimit;
	const float upper = (upperLimit < 0.0f) ? length : upperLimit;
	const float lineTop = (lineTopY < 0.0f) ? (top + PinHeadHeight) : lineTopY;

	// 頭の位置につかめる領域を置くため、カーソルを一時的に動かす
	const ImVec2 savedCursor = ImGui::GetCursorScreenPos();
	bool changed = false;

	ImGui::PushID(id);
	for (int i = 0; i < 2; i++)
	{
		const bool isStart = (i == 0);
		float& value = isStart ? inOutStart : inOutEnd;

		ImGui::PushID(i);
		ImGui::SetCursorScreenPos(ImVec2(TimeToX(value) - RangeHandleHalfWidth, handleY));
		ImGui::InvisibleButton("##RangeHandle", ImVec2(RangeHandleHalfWidth * 2.0f, RangeHandleHeight));
		const bool active = ImGui::IsItemActive();
		const bool hovered = ImGui::IsItemHovered();
		ImGui::PopID();

		// ドラッグで動かす
		if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			float t = SnapToFrame(XToTime(ImGui::GetIO().MousePos.x), frameStep, length);

			// もう一方の端と、最低 minLength は空ける
			if (isStart) t = (std::min)(t, inOutEnd - minLength);
			else         t = (std::max)(t, inOutStart + minLength);
			t = std::clamp(t, lower, upper);

			if (t != value)
			{
				value = t;
				changed = true;
			}
		}

		DrawRangeHandle(drawList, TimeToX(value), handleY, lineTop, active || hovered);
	}
	ImGui::PopID();

	ImGui::SetCursorScreenPos(savedCursor);
	return changed;
}

// ピンを1本描く
void StudioTimelineWidget::DrawPin(ImDrawList* drawList, float x, float tipY, float headHeight, float radius, ImU32 color, PinDirection direction)
{
	// 頭の丸の中心と、三角形の底辺の位置
	const float sign = (direction == PinDirection::Down) ? -1.0f : 1.0f;
	const float headY = tipY + sign * (headHeight - radius);
	const float baseY = headY - sign * 2.0f;

	drawList->AddTriangleFilled(
		ImVec2(x - radius * 0.8f, baseY),
		ImVec2(x + radius * 0.8f, baseY),
		ImVec2(x, tipY),
		color);
	drawList->AddCircleFilled(ImVec2(x, headY), radius, color);
}

// 時刻とX座標の変換
float StudioTimelineWidget::TimeToX(float t) const
{
	return left + (t / length) * width;
}
float StudioTimelineWidget::XToTime(float x) const
{
	return std::clamp((x - left) / width * length, 0.0f, length);
}


// 目盛り
void StudioTimelineWidget::DrawRuler(ImDrawList* drawList, float frameStep) const
{
	const float rulerTop = top + PinHeadHeight;
	const float bottom = rulerTop + RulerHeight;
	drawList->AddRectFilled(
		ImVec2(left - LeftPadding, rulerTop),
		ImVec2(left + width + RightPadding, bottom),
		RulerBgColor);

	// フレームごとの細かい目盛り
	if (frameStep > 0.0f && width * frameStep / length >= 4.0f)
	{
		const int frameCount = (int)std::round(length / frameStep);
		for (int i = 0; i <= frameCount; i++)
		{
			const float x = TimeToX(i * frameStep);
			drawList->AddLine(ImVec2(x, bottom - 4.0f), ImVec2(x, bottom), TickColor);
		}
	}

	// 秒の目盛り
	const float candidates[] = { 0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f };
	float major = candidates[6];
	for (float c : candidates)
	{
		if (width * c / length >= 60.0f)
		{
			major = c;
			break;
		}
	}

	const int majorCount = (int)std::floor(length / major + 0.0001f);
	for (int i = 0; i <= majorCount; i++)
	{
		const float t = i * major;
		const float x = TimeToX(t);
		drawList->AddLine(ImVec2(x, bottom - 12.0f), ImVec2(x, bottom), TickColor);

		char label[16];
		snprintf(label, sizeof(label), "%.1f", t);
		drawList->AddText(ImVec2(x + 3.0f, rulerTop + 2.0f), LabelColor, label);
	}
}

// ピンの頭
void StudioTimelineWidget::DrawPinHead(ImDrawList* drawList) const
{
	// 再生位置のピン：下向き、とがった先が目盛りの帯の上端
	DrawPin(drawList, TimeToX(time), top + PinHeadHeight, PinHeadHeight, 6.0f, PinColor, PinDirection::Down);
}
void StudioTimelineWidget::DrawRangeHandle(ImDrawList* drawList, float x, float handleY, float lineTopY, bool highlighted) const
{
	const ImU32 color = highlighted ? RangeHandleActiveColor : RangeHandleColor;

	// 線：頭のとがった先から、指定された上端まで
	drawList->AddLine(ImVec2(x, handleY), ImVec2(x, lineTopY), color, 1.5f);

	// 頭：上向き、とがった先が帯の下端
	DrawPin(drawList, x, handleY, RangeHandleHeight, 6.0f, color, PinDirection::Up);
}
