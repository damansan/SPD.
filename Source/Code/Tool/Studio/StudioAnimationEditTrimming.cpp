#include "StudioAnimationEdit.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/Resource/Model.h"
#include <algorithm>
#include <cmath>

// トリミングのトラック
bool StudioAnimationEdit::DrawTrimTrack(const std::string& clipName, float rawLength, float frameStep)
{
	float start = 0.0f;
	float end = 0.0f;
	GetTrimRange(clipName, rawLength, start, end);

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const float margin = 4.0f;
	const float trackHeight = 28.0f;
	const float top = ImGui::GetCursorScreenPos().y + margin;
	const float bottom = top + trackHeight;

	// 元の長さ全体（暗い帯）
	drawList->AddRectFilled(ImVec2(timeline.GetLeft(), top), ImVec2(timeline.GetRight(), bottom), IM_COL32(50, 52, 56, 255), 3.0f);
	// 使う範囲（明るい帯）
	drawList->AddRectFilled(ImVec2(timeline.TimeToX(start), top), ImVec2(timeline.TimeToX(end), bottom), IM_COL32(79, 195, 247, 200), 3.0f);

	// 範囲の端のピン
	const float minLength = StudioTimelineWidget::MinLength(frameStep);
	bool changed = false;
	if (timeline.DrawRangeHandles("##TrimRange", bottom, frameStep, minLength, start, end))
	{
		changed = SetTrimRange(clipName, rawLength, start, end);
	}

	// 帯とピンの頭の分の高さを確保する
	ImGui::Dummy(ImVec2(0.0f, margin + trackHeight + StudioTimelineWidget::RangeHandleHeight + margin));

	return changed;
}

// トリミングのボタン
bool StudioAnimationEdit::DrawTrimButtons(const std::string& clipName, float rawLength, float frameStep, float pinTime)
{
	float start = 0.0f;
	float end = 0.0f;
	GetTrimRange(clipName, rawLength, start, end);

	// 最低1フレーム分の長さを残す
	const float minLength = StudioTimelineWidget::MinLength(frameStep);
	const float tolerance = minLength * 0.5f;

	bool changed = false;

	ImGui::BeginDisabled(pinTime > end - minLength + tolerance);
	if (ImGui::Button("Set Start"))
	{
		changed |= SetTrimRange(clipName, rawLength, pinTime, end);
	}
	ImGui::EndDisabled();

	ImGui::SameLine();

	ImGui::BeginDisabled(pinTime < start + minLength - tolerance);
	if (ImGui::Button("Set End"))
	{
		changed |= SetTrimRange(clipName, rawLength, start, pinTime);
	}
	ImGui::EndDisabled();

	return changed;
}

// トリミングの数値
bool StudioAnimationEdit::DrawTrimSettings(const Model& model, AnimationEditClipData& clip)
{
	const float rawLength = FindRawLength(model, clip.clipName);
	const float frameStep = CalcFrameStep(model, clip.clipName);
	const float minLength = StudioTimelineWidget::MinLength(frameStep);
	const float dragSpeed = minLength; // 1回のドラッグで1フレームずつ動かす

	float start = clip.trimStart;
	float end = (clip.trimEnd < 0.0f) ? rawLength : clip.trimEnd;

	bool changed = false;
	changed |= ImGui::DragFloat("Start", &start, dragSpeed, 0.0f, end - minLength, "%.3f s");
	changed |= ImGui::DragFloat("End", &end, dragSpeed, start + minLength, rawLength, "%.3f s");

	if (changed)
	{
		// フレームに吸い付かせてから書き戻す
		start = StudioTimelineWidget::SnapToFrame(start, frameStep, rawLength);
		end = StudioTimelineWidget::SnapToFrame(end, frameStep, rawLength);
		clip.trimStart = start;
		clip.trimEnd = (end >= rawLength - 0.0001f) ? -1.0f : end; // 最後までなら「最後まで」として保存
	}

	// フレームと長さの表示
	ImGui::TextDisabled("F%d - F%d", StudioTimelineWidget::ToFrame(start, frameStep), StudioTimelineWidget::ToFrame(end, frameStep));
	ImGui::TextDisabled("Length %.3f s (raw %.3f s)", end - start, rawLength);

	return changed;
}

// 使う範囲の中で時刻を進める
void StudioAnimationEdit::AdvanceRangePlayback(AnimatorComponent& animator, StudioContext& context, float start, float end)
{
	const float rangeLength = end - start;
	if (rangeLength <= 0.0f)
	{
		rangePlaying = false;
		return;
	}

	// 別のパネルの再生が始まったら、そちらを優先して止まる
	if (animator.isPlaying)
	{
		rangePlaying = false;
		return;
	}

	float t = animator.animationTime + ImGui::GetIO().DeltaTime * animator.playSpeed;

	// 範囲の終わりを越えたら、越えた分を残して Start 側へ戻す
	if (t >= end) t = start + std::fmod(t - start, rangeLength);
	if (t < start) t = start;

	animator.animationTime = t;
	context.requestPoseUpdate = true;
}

// 範囲を取り出す
void StudioAnimationEdit::GetTrimRange(const std::string& clipName, float rawLength, float& outStart, float& outEnd) const
{
	outStart = 0.0f;
	outEnd = rawLength;

	if (const AnimationEditClipData* clip = editData.FindClip(clipName))
	{
		outStart = std::clamp(clip->trimStart, 0.0f, rawLength);
		outEnd = (clip->trimEnd < 0.0f) ? rawLength : std::clamp(clip->trimEnd, outStart, rawLength);
	}
}

// 範囲を書き込む
bool StudioAnimationEdit::SetTrimRange(const std::string& clipName, float rawLength, float start, float end)
{
	// 最後までなら「-1（最後まで）」として保存する
	const float storedEnd = (end >= rawLength - 0.0001f) ? -1.0f : end;

	// 変わらないなら、設定を作らずに終わる
	const AnimationEditClipData* existing = editData.FindClip(clipName);
	const float oldStart = existing ? existing->trimStart : 0.0f;
	const float oldEnd = existing ? existing->trimEnd : -1.0f;
	if (oldStart == start && oldEnd == storedEnd) return false;

	AnimationEditClipData* clip = editData.FindOrCreateClip(clipName);
	clip->trimStart = start;
	clip->trimEnd = storedEnd;
	return true;
}
