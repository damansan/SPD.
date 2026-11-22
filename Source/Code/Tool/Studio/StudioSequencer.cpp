#include "StudioSequencer.h"
#include "Engine/ECS/Components/SkinnedMeshComponent.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include <algorithm>
#include <cmath>

namespace
{
	// Point / Range の色
	const ImU32 ItemColor =     IM_COL32(129, 199, 132, 255);
	const ImU32 ItemDimColor =  IM_COL32(129, 199, 132, 140);
	const ImU32 SelectedColor = IM_COL32(255, 213, 79, 255); 

	// 行の大きさと色
	const float LaneMargin = 2.0f;
	const float PointLaneHeight = 28.0f;
	const float RangeLaneHeight = 24.0f;
	const float CollapsedLaneHeight = 14.0f;
	const ImU32 LaneColor = IM_COL32(43, 45, 49, 255);
	const ImU32 LaneSelectedColor = IM_COL32(56, 58, 64, 255);

	// 行の位置
	struct Lane
	{
		float cursorX = 0.0f; // 描き始めたときのカーソルの X 位置
		float top = 0.0f;
		float bottom = 0.0f;
	};
	
	// 行の背景を描いて、位置を返す
	Lane BeginLane(const StudioTimelineWidget& timeline, float height, ImU32 color)
	{
		const ImVec2 cursor = ImGui::GetCursorScreenPos();
		Lane lane;
		lane.cursorX = cursor.x;
		lane.top = cursor.y + LaneMargin;
		lane.bottom = lane.top + height;

		ImGui::GetWindowDrawList()->AddRectFilled(
			ImVec2(timeline.GetLeft(), lane.top), ImVec2(timeline.GetRight(), lane.bottom), color, 3.0f);
		return lane;
	}

	// 行の下にカーソルを進める
	void EndLane(const Lane& lane, float extra)
	{
		ImGui::SetCursorScreenPos(ImVec2(lane.cursorX, lane.bottom + extra + LaneMargin));
		ImGui::Dummy(ImVec2(0.0f, 0.0f));
	}

	// カットされた部分にないか（Point）
	bool IsPointVisible(const Model::Animation& animation, float rawTime)
	{
		const float rawStart = animation.ToRawTime(0.0f);
		const float rawEnd = animation.ToRawTime(animation.secondsLength);
		return rawTime >= rawStart - 0.0001f && rawTime <= rawEnd + 0.0001f;
	}

	// 全部がカットされた部分にないか（Range）
	bool IsRangeVisible(const Model::Animation& animation, float rawStartTime, float rawEndTime)
	{
		const float rawStart = animation.ToRawTime(0.0f);
		const float rawEnd = animation.ToRawTime(animation.secondsLength);
		return rawEndTime > rawStart + 0.0001f && rawStartTime < rawEnd - 0.0001f;
	}

	// 同じトラックの Range を、タイムラインの時刻の範囲の並びにする
	std::vector<StudioTimelineWidget::TimeSpan> CollectSpans(const SequencerClipData& clip, SequencerFlag flag, const Model::Animation& animation, int targetId, int& outTargetIndex)
	{
		std::vector<StudioTimelineWidget::TimeSpan> spans;
		outTargetIndex = -1;

		for (const auto& r : clip.ranges)
		{
			if (r.flag != flag) continue;
			if (r.id == targetId) outTargetIndex = (int)spans.size();
			spans.push_back({ animation.ToBakedTime(r.start), animation.ToBakedTime(r.end) });
		}
		return spans;
	}

	// Range が動ける範囲
	void GetRangeLimits(const SequencerClipData& clip, const SequencerRange& target, const Model::Animation& animation, float& outLower, float& outUpper)
	{
		int index = -1;
		const auto spans = CollectSpans(clip, target.flag, animation, target.id, index);
		StudioTimelineWidget::GetSpanLimits(spans, index, animation.secondsLength, outLower, outUpper);
	}

	// id から Range を探す
	SequencerRange* FindRange(SequencerClipData* clip, int id)
	{
		if (!clip) return nullptr;
		for (auto& r : clip->ranges)
		{
			if (r.id == id) return &r;
		}
		return nullptr;
	}

	// id から Point を探す
	SequencerPoint* FindPoint(SequencerClipData* clip, int id)
	{
		if (!clip) return nullptr;
		for (auto& p : clip->points)
		{
			if (p.id == id) return &p;
		}
		return nullptr;
	}
}

// ImGui描画
void StudioSequencer::DrawGUI(entt::registry& registry, StudioContext& context)
{
	if (!context.showSequencer) return;

	std::string panelName = "Sequencer##" + std::to_string(windowId);
	ImGui::Begin(panelName.c_str());

	auto view = registry.view<SkinnedMeshComponent, AnimatorComponent>();
	if (view.begin() == view.end())
	{
		ImGui::Text("No skinned model.");
		ImGui::End();
		return;
	}

	auto entity = *view.begin();
	auto& skinned = view.get<SkinnedMeshComponent>(entity);
	auto& animator = view.get<AnimatorComponent>(entity);

	if (!skinned.model)
	{
		ImGui::End();
		return;
	}

	// モデルが切り替わったら、そのモデル用のファイルを読み込み直す
	if (loadedModelPath != context.currentModelPath)
	{
		Load(context.currentModelPath);
	}

	if (animator.currentAnimation.empty())
	{
		ImGui::TextDisabled("Select an animation from the Animator panel.");
		ImGui::End();
		return;
	}

	// クリップが変わったら、Point / Range の選択を外す
	if (selectedClipName != animator.currentAnimation)
	{
		selectedClipName = animator.currentAnimation;
		selectedType = SelectionType::None;
		selectedId = -1;
	}

	const std::string& clipName = animator.currentAnimation;
	const float detailWidth = 260.0f;

	// 左：タイムライン
	ImGui::BeginChild("SequencerTimeline", ImVec2(-detailWidth, 0.0f));
	bool changed = DrawTimelineArea(*skinned.model, animator, context);
	ImGui::EndChild();

	ImGui::SameLine();

	// 右：詳細
	ImGui::BeginChild("SequencerDetail", ImVec2(detailWidth, 0.0f), true);
	changed |= DrawDetailArea(*skinned.model, clipName, context.currentModelPath);
	ImGui::EndChild();

	if (changed)
	{
		isDirty = true;
	}

	ImGui::End();
}

// タイムライン
bool StudioSequencer::DrawTimelineArea(const Model& model, AnimatorComponent& animator, StudioContext& context)
{
	const std::string& clipName = animator.currentAnimation;

	// 焼き込み後のクリップの長さ
	const Model::Animation* animation = model.FindAnimation(clipName);
	const float length = animation ? animation->secondsLength : 0.0f;

	// 1フレームの秒数は生データから求める
	const Model::Animation* raw = model.FindRawAnimation(clipName);
	const float frameStep = raw ? raw->CalcFrameStep() : 0.0f;

	const bool preview = animator.usePreview && animation;
	auto toViewTime = [&](float animatorTime) { return preview ? animation->ToBakedTime(animatorTime) : animatorTime; };
	auto toAnimatorTime = [&](float viewTime) { return preview ? animation->ToRawTime(viewTime) : viewTime; };

	// タイムラインの時刻
	float viewTime = toViewTime(animator.animationTime);

	// 再生ボタンと現在時刻
	if (StudioTimelineWidget::DrawToolbar(animator.isPlaying, viewTime, frameStep))
	{
		// 最後まで再生しきった状態から再生したら、頭に戻す
		if (animator.isPlaying && viewTime >= length)
		{
			animator.animationTime = toAnimatorTime(0.0f);
		}
		animator.isEnd = false;
	}

	// タイムライン
	if (timeline.Begin("##SequencerRuler", length, frameStep, viewTime))
	{
		animator.animationTime = toAnimatorTime(viewTime);
		animator.isPlaying = false;
		animator.isEnd = false;
		context.requestPoseUpdate = true;
	}

	// フラグごとのトラック
	bool changed = false;
	for (int i = 0; i < (int)SequencerFlag::Count; i++)
	{
		changed |= DrawTrack((SequencerFlag)i, model, clipName, frameStep);
	}

	timeline.End();

	// 再生位置で操作するボタン
	const float pinTime = StudioTimelineWidget::SnapToFrame(viewTime, frameStep, length);
	changed |= DrawItemButtons(model, clipName, pinTime, frameStep);

	return changed;
}

// 数値
bool StudioSequencer::DrawDetailArea(const Model& model, const std::string& clipName, const std::string& modelPath)
{
	ImGui::Text("Clip: %s", clipName.c_str());
	ImGui::TextDisabled("Track: %s", GetFlagName(selectedTrack));
	ImGui::Separator();

	// 選んでいる Point / Range の詳細
	bool changed = DrawSelectedItem(model, clipName);

	ImGui::Separator();

	// 保存
	if (ImGui::Button("Save"))
	{
		Save(modelPath);
	}
	if (isDirty)
	{
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "* unsaved");
	}

	return changed;
}
// 詳細
bool StudioSequencer::DrawSelectedItem(const Model& model, const std::string& clipName)
{
	const Model::Animation* animation = model.FindAnimation(clipName);
	SequencerClipData* clip = sequencerData.FindClip(clipName);
	SequencerPoint* point = (selectedType == SelectionType::Point) ? FindPoint(clip, selectedId) : nullptr;
	SequencerRange* range = (selectedType == SelectionType::Range) ? FindRange(clip, selectedId) : nullptr;

	if (!animation || (!point && !range))
	{
		ImGui::TextDisabled("No item selected.");
		return false;
	}

	const Model::Animation* raw = model.FindRawAnimation(clipName);
	const float frameStep = raw ? raw->CalcFrameStep() : 0.0f;
	const float length = animation->secondsLength;
	const float minLength = StudioTimelineWidget::MinLength(frameStep);
	bool changed = false;

	ImGui::Text(point ? "Point" : "Range");

	// 別のトラックへ移す
	const SequencerFlag currentFlag = point ? point->flag : range->flag;
	if (ImGui::BeginCombo("Track", GetFlagName(currentFlag)))
	{
		for (int i = 0; i < (int)SequencerFlag::Count; i++)
		{
			const SequencerFlag flag = (SequencerFlag)i;
			const bool isCurrent = (flag == currentFlag);

			// Range は、移した先の Range と重なるトラックは選べない
			bool blocked = false;
			if (range && !isCurrent)
			{
				const float start = animation->ToBakedTime(range->start);
				const float end = animation->ToBakedTime(range->end);
				int unusedIndex = -1;
				for (const auto& span : CollectSpans(*clip, flag, *animation, -1, unusedIndex))
				{
					if (start < span.end - 0.0001f && end > span.start + 0.0001f)
					{
						blocked = true;
						break;
					}
				}
			}

			ImGui::BeginDisabled(blocked);
			if (ImGui::Selectable(GetFlagName(flag), isCurrent) && !isCurrent)
			{
				if (point) point->flag = flag;
				else       range->flag = flag;

				selectedTrack = flag; // 移した先のトラックを開く
				changed = true;
			}
			ImGui::EndDisabled();

			if (isCurrent) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}


	// 時刻
	if (point)
	{
		float time = animation->ToBakedTime(point->time);
		if (ImGui::DragFloat("Time", &time, minLength, 0.0f, length, "%.3f s"))
		{
			time = StudioTimelineWidget::SnapToFrame(time, frameStep, length);
			point->time = animation->ToRawTime(time);
			changed = true;
		}
		ImGui::TextDisabled("F%d", StudioTimelineWidget::ToFrame(animation->ToBakedTime(point->time), frameStep));
	}
	else
	{
		float start = animation->ToBakedTime(range->start);
		float end = animation->ToBakedTime(range->end);

		// 同じトラックの隣の Range にぶつからない範囲
		float lower = 0.0f;
		float upper = length;
		GetRangeLimits(*clip, *range, *animation, lower, upper);

		bool rangeChanged = false;
		rangeChanged |= ImGui::DragFloat("Start", &start, minLength, lower, end - minLength, "%.3f s");
		rangeChanged |= ImGui::DragFloat("End", &end, minLength, start + minLength, upper, "%.3f s");

		if (rangeChanged)
		{
			// フレームに吸い付かせてから、隣の Range と最低の長さを守る
			start = std::clamp(StudioTimelineWidget::SnapToFrame(start, frameStep, length), lower, end - minLength);
			end = std::clamp(StudioTimelineWidget::SnapToFrame(end, frameStep, length), start + minLength, upper);

			range->start = animation->ToRawTime(start);
			range->end = animation->ToRawTime(end);
			changed = true;
		}
		ImGui::TextDisabled("F%d - F%d",
			StudioTimelineWidget::ToFrame(animation->ToBakedTime(range->start), frameStep),
			StudioTimelineWidget::ToFrame(animation->ToBakedTime(range->end), frameStep));
	}

	// 削除
	if (ImGui::Button("Delete"))
	{
		const int id = selectedId;
		if (point)
		{
			auto& list = clip->points;
			list.erase(std::remove_if(list.begin(), list.end(), [id](const SequencerPoint& p) { return p.id == id; }), list.end());
		}
		else
		{
			auto& list = clip->ranges;
			list.erase(std::remove_if(list.begin(), list.end(), [id](const SequencerRange& r) { return r.id == id; }), list.end());
		}

		// 消したものを指したままにならないように、選択を外す
		selectedType = SelectionType::None;
		selectedId = -1;
		changed = true;
	}

	return changed;
}

// トラックの描画
bool StudioSequencer::DrawTrack(SequencerFlag flag, const Model& model, const std::string& clipName, float frameStep)
{
	const int index = (int)flag;
	const bool trackSelected = (selectedTrack == flag);

	// 見出し：フラグ名
	ImGui::PushID(index);
	ImGui::SetCursorScreenPos(ImVec2(timeline.GetLeft(), ImGui::GetCursorScreenPos().y));

	const char* name = GetFlagName(flag);
	if (ImGui::Selectable(name, trackSelected, 0, ImVec2(ImGui::CalcTextSize(name).x, 0.0f)))
	{
		selectedTrack = flag;
		selectedType = SelectionType::None;
		selectedId = -1;
	}
	ImGui::PopID();

	// 行（選んでいるフラグだけ開く）
	bool changed = false;
	if (trackSelected)
	{
		changed |= DrawPointLane(flag, model, clipName, frameStep);
		changed |= DrawRangeLane(flag, model, clipName, frameStep);
	}
	else
	{
		DrawCollapsedLane(flag, model, clipName);
	}
	return changed;
}

// Point の行
bool StudioSequencer::DrawPointLane(SequencerFlag flag, const Model& model, const std::string& clipName, float frameStep)
{
	const Lane lane = BeginLane(timeline, PointLaneHeight, (selectedTrack == flag) ? LaneSelectedColor : LaneColor);
	bool changed = false;

	const Model::Animation* animation = model.FindAnimation(clipName);
	SequencerClipData* clip = sequencerData.FindClip(clipName);

	if (animation && clip)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const float length = animation->secondsLength;
		const float halfWidth = 7.0f;          // つかめる幅の半分
		const float tipY = lane.top + 14.0f;   // ピンのとがった先

		ImGui::PushID((int)flag);
		ImGui::PushID("Points");
		for (auto& p : clip->points)
		{
			if (p.flag != flag || !IsPointVisible(*animation, p.time)) continue;

			const float x = timeline.TimeToX(animation->ToBakedTime(p.time));

			ImGui::PushID(p.id);
			ImGui::SetCursorScreenPos(ImVec2(x - halfWidth, lane.top));
			ImGui::InvisibleButton("##Point", ImVec2(halfWidth * 2.0f, lane.bottom - lane.top));

			// 押した瞬間に選ぶ
			if (ImGui::IsItemActivated())
			{
				selectedTrack = flag;
				selectedType = SelectionType::Point;
				selectedId = p.id;
			}

			// ドラッグで動かす
			if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
			{
				const float t = StudioTimelineWidget::SnapToFrame(timeline.XToTime(ImGui::GetIO().MousePos.x), frameStep, length);
				const float raw = animation->ToRawTime(t);
				if (raw != p.time)
				{
					p.time = raw;
					changed = true;
				}
			}
			ImGui::PopID();

			// ピン
			const bool selected = (selectedType == SelectionType::Point && selectedId == p.id);
			const ImU32 color = selected ? SelectedColor : ItemColor;
			const float drawX = timeline.TimeToX(animation->ToBakedTime(p.time));

			drawList->AddLine(ImVec2(drawX, tipY), ImVec2(drawX, lane.bottom), color, 1.5f);
			StudioTimelineWidget::DrawPin(drawList, drawX, tipY, 12.0f, 4.5f, color, StudioTimelineWidget::PinDirection::Down);
		}
		ImGui::PopID();
		ImGui::PopID();
	}

	EndLane(lane, 0.0f);
	return changed;
}

// Range の行
bool StudioSequencer::DrawRangeLane(SequencerFlag flag, const Model& model, const std::string& clipName, float frameStep)
{
	const Lane lane = BeginLane(timeline, RangeLaneHeight, (selectedTrack == flag) ? LaneSelectedColor : LaneColor);
	bool changed = false;

	const Model::Animation* animation = model.FindAnimation(clipName);
	SequencerClipData* clip = sequencerData.FindClip(clipName);

	if (animation && clip)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const float length = animation->secondsLength;
		const float centerY = (lane.top + lane.bottom) * 0.5f;

		ImGui::PushID((int)flag);
		ImGui::PushID("Ranges");
		for (auto& r : clip->ranges)
		{
			if (r.flag != flag || !IsRangeVisible(*animation, r.start, r.end)) continue;

			// カットの範囲からはみ出した部分は、端に寄せて描く
			const float x0 = timeline.TimeToX(animation->ToBakedTime(r.start));
			const float x1 = timeline.TimeToX(animation->ToBakedTime(r.end));
			const bool selected = (selectedType == SelectionType::Range && selectedId == r.id);

			// クリックで選ぶ（帯をつかんで動かす操作は無し）
			ImGui::PushID(r.id);
			ImGui::SetCursorScreenPos(ImVec2(x0, lane.top));
			ImGui::InvisibleButton("##Range", ImVec2((std::max)(x1 - x0, 1.0f), lane.bottom - lane.top));
			if (ImGui::IsItemActivated())
			{
				selectedTrack = flag;
				selectedType = SelectionType::Range;
				selectedId = r.id;
			}
			ImGui::PopID();

			// 帯
			drawList->AddRectFilled(ImVec2(x0, lane.top + 3.0f), ImVec2(x1, lane.bottom - 3.0f), selected ? ItemColor : ItemDimColor, 3.0f);

			// フラグ名（入りきる幅があるときだけ）
			const char* name = GetFlagName(flag);
			const ImVec2 textSize = ImGui::CalcTextSize(name);
			if (textSize.x + 6.0f < x1 - x0)
			{
				drawList->AddText(ImVec2((x0 + x1 - textSize.x) * 0.5f, centerY - textSize.y * 0.5f), IM_COL32(0, 0, 0, 255), name);
			}
		}
		ImGui::PopID();

		// 選んでいる Range の端のピン（このトラックの Range を選んでいるときだけ）
		if (selectedType == SelectionType::Range && selectedTrack == flag)
		{
			SequencerRange* r = FindRange(clip, selectedId);
			if (r && r->flag == flag)
			{
				float start = animation->ToBakedTime(r->start);
				float end = animation->ToBakedTime(r->end);

				// 同じトラックの隣の Range にぶつかったら止まる
				float lower = 0.0f;
				float upper = length;
				GetRangeLimits(*clip, *r, *animation, lower, upper);

				const float minLength = StudioTimelineWidget::MinLength(frameStep);
				if (timeline.DrawRangeHandles("##RangeHandles", lane.bottom, frameStep, minLength, start, end, lower, upper, lane.top))
				{
					// 生データの秒に戻して書き込む
					r->start = animation->ToRawTime(start);
					r->end = animation->ToRawTime(end);
					changed = true;
				}
			}
		}
		ImGui::PopID();
	}

	// 帯と、Range の端のピンの分の高さを確保する
	EndLane(lane, StudioTimelineWidget::RangeHandleHeight);
	return changed;
}

// 閉じているときの細い行
void StudioSequencer::DrawCollapsedLane(SequencerFlag flag, const Model& model, const std::string& clipName)
{
	const Lane lane = BeginLane(timeline, CollapsedLaneHeight, LaneColor);

	const Model::Animation* animation = model.FindAnimation(clipName);
	SequencerClipData* clip = sequencerData.FindClip(clipName);

	if (animation && clip)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const float midY = (lane.top + lane.bottom) * 0.5f;

		// 上半分：Point を短い縦線で
		for (const auto& p : clip->points)
		{
			if (p.flag != flag || !IsPointVisible(*animation, p.time)) continue;
			const float x = timeline.TimeToX(animation->ToBakedTime(p.time));
			drawList->AddLine(ImVec2(x, lane.top + 2.0f), ImVec2(x, midY - 1.0f), ItemColor, 1.5f);
		}

		// 下半分：Range を細い帯で
		for (const auto& r : clip->ranges)
		{
			if (r.flag != flag || !IsRangeVisible(*animation, r.start, r.end)) continue;
			const float x0 = timeline.TimeToX(animation->ToBakedTime(r.start));
			const float x1 = timeline.TimeToX(animation->ToBakedTime(r.end));
			drawList->AddRectFilled(ImVec2(x0, midY + 1.0f), ImVec2(x1, lane.bottom - 2.0f), ItemDimColor, 2.0f);
		}
	}

	EndLane(lane, 0.0f);
}

// ボタン
bool StudioSequencer::DrawItemButtons(const Model& model, const std::string& clipName, float pinTime, float frameStep)
{
	const Model::Animation* animation = model.FindAnimation(clipName);
	if (!animation) return false;

	const float length = animation->secondsLength;
	const float minLength = StudioTimelineWidget::MinLength(frameStep);
	const float tolerance = minLength * 0.5f; // 小数の誤差として許す量

	// 再生位置を、生データの秒に直す
	const float rawPin = animation->ToRawTime(pinTime);

	SequencerClipData* clip = sequencerData.FindClip(clipName);
	bool changed = false;

	// Point 追加
	if (ImGui::Button("Add Point"))
	{
		clip = sequencerData.FindOrCreateClip(clipName);

		SequencerPoint p;
		p.id = clip->GenerateId();
		p.time = rawPin;
		p.flag = selectedTrack;
		clip->points.push_back(p);

		selectedType = SelectionType::Point;
		selectedId = p.id;
		changed = true;
	}

	// Range追加
	int unusedIndex = -1;
	const std::vector<StudioTimelineWidget::TimeSpan> spans = clip
		? CollectSpans(*clip, selectedTrack, *animation, -1, unusedIndex)
		: std::vector<StudioTimelineWidget::TimeSpan>{};

	float addEnd = length;
	const bool canAdd = StudioTimelineWidget::FindAddEnd(spans, pinTime, length, minLength, addEnd);

	ImGui::SameLine();
	ImGui::BeginDisabled(!canAdd);
	if (ImGui::Button("Add Range"))
	{
		clip = sequencerData.FindOrCreateClip(clipName);

		SequencerRange r;
		r.id = clip->GenerateId();
		r.start = rawPin;
		r.end = animation->ToRawTime(addEnd);
		r.flag = selectedTrack;
		clip->ranges.push_back(r);

		selectedType = SelectionType::Range;
		selectedId = r.id;
		changed = true;
	}
	ImGui::EndDisabled();

	// 選んでいるものを再生位置に合わせる
	if (selectedType == SelectionType::Point)
	{
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(16.0f, 0.0f)); // 追加ボタンとの間を空ける
		ImGui::SameLine();

		if (ImGui::Button("Set Time"))
		{
			if (SequencerPoint* p = FindPoint(clip, selectedId))
			{
				p->time = rawPin;
				changed = true;
			}
		}
	}
	else if (selectedType == SelectionType::Range)
	{
		SequencerRange* r = FindRange(clip, selectedId);

		float start = 0.0f;
		float end = 0.0f;
		float lower = 0.0f;
		float upper = length;
		if (r)
		{
			start = animation->ToBakedTime(r->start);
			end = animation->ToBakedTime(r->end);
			GetRangeLimits(*clip, *r, *animation, lower, upper);
		}

		ImGui::SameLine();
		ImGui::Dummy(ImVec2(16.0f, 0.0f));
		ImGui::SameLine();

		// Set Start：前の Range に食い込まず、終わりとの間を1フレーム残す
		ImGui::BeginDisabled(!r || pinTime < lower - tolerance || pinTime > end - minLength + tolerance);
		if (ImGui::Button("Set Start"))
		{
			r->start = rawPin;
			changed = true;
		}
		ImGui::EndDisabled();

		ImGui::SameLine();

		// Set End：後ろの Range に食い込まず、始まりとの間を1フレーム残す
		ImGui::BeginDisabled(!r || pinTime > upper + tolerance || pinTime < start + minLength - tolerance);
		if (ImGui::Button("Set End"))
		{
			r->end = rawPin;
			changed = true;
		}
		ImGui::EndDisabled();
	}

	return changed;
}

// フラグの表示名
const char* StudioSequencer::GetFlagName(SequencerFlag flag)
{
	// 種類が増えたらここにも追加する
	static const char* names[] =
	{
		"AnimExit",
	};

	// names の要素数と、SequencerFlag の個数が食い違っていたら、ビルド時にエラーにする
	static_assert(sizeof(names) / sizeof(names[0]) == (size_t)SequencerFlag::Count,
		"SequencerFlag と names の数が一致しません");

	const int index = (int)flag;
	if (index < 0 || index >= (int)SequencerFlag::Count) return "?";
	return names[index];
}

// 読み込み
void StudioSequencer::Load(const std::string& modelPath)
{
	// 無い・壊れているときは初期値が返る。無くて普通のファイルなので、ログは出さない
	sequencerData = fileCache.GetOrLoad(SequencerData::GetFilePath(modelPath),
		[](SequencerData& data, const nlohmann::json& j) { data.FromJson(j); }, JsonMissingLog::Silent);

	loadedModelPath = modelPath;
	isDirty = false;
	selectedType = SelectionType::None;
	selectedId = -1;
}
// 保存
void StudioSequencer::Save(const std::string& modelPath)
{
	if (modelPath.empty()) return;

	if (fileCache.Save(SequencerData::GetFilePath(modelPath), sequencerData,
		[](const SequencerData& data) { return data.ToJson(); }))
	{
		isDirty = false;
	}
}