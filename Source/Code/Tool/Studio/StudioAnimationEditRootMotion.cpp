#include "StudioAnimationEdit.h"
#include "Engine/Resource/AnimationEditBaker.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
	// rootの位置キーから、グラフ用の値を作る
	RootMotionCurve BuildCurve(const Model::Animation& animation, int rootIndex)
	{
		RootMotionCurve curve;
		if (rootIndex < 0 || rootIndex >= (int)animation.nodeAnims.size()) return curve;

		const auto& keys = animation.nodeAnims[rootIndex].positionKeyframes;
		if (keys.empty()) return curve;

		float distance = 0.0f;
		for (size_t i = 0; i < keys.size(); i++)
		{
			// 水平に進んだ距離を積み上げる
			if (i > 0)
			{
				const float dx = keys[i].value.x - keys[i - 1].value.x;
				const float dz = keys[i].value.z - keys[i - 1].value.z;
				distance += std::sqrt(dx * dx + dz * dz);
			}

			curve.times.push_back(keys[i].seconds);
			curve.xz.push_back(distance);
			curve.y.push_back(keys[i].value.y - keys.front().value.y);
		}
		return curve;
	}

	// 点線を描く
	void AddDashedPolyline(ImDrawList* drawList, const std::vector<ImVec2>& points, ImU32 color, float thickness)
	{
		const float dash = 6.0f;
		const float gap = 4.0f;
		const float period = dash + gap;
		float pattern = 0.0f; // 点線の模様のどこまで進んだか

		for (size_t i = 1; i < points.size(); i++)
		{
			const ImVec2 a = points[i - 1];
			const ImVec2 b = points[i];
			const float segLength = std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
			if (segLength <= 0.0f) continue;

			float done = 0.0f;
			while (done < segLength)
			{
				const float phase = std::fmod(pattern, period);
				const bool drawing = (phase < dash);
				const float remain = drawing ? (dash - phase) : (period - phase);
				const float step = (std::min)(remain, segLength - done);

				if (drawing)
				{
					const float t0 = done / segLength;
					const float t1 = (done + step) / segLength;
					drawList->AddLine(
						ImVec2(a.x + (b.x - a.x) * t0, a.y + (b.y - a.y) * t0),
						ImVec2(a.x + (b.x - a.x) * t1, a.y + (b.y - a.y) * t1),
						color, thickness);
				}

				done += step;
				pattern += step;
			}
		}
	}

	// グラフの値を、指定時刻で補間して取り出す
	float SampleCurve(const std::vector<float>& times, const std::vector<float>& values, float t)
	{
		if (values.empty()) return 0.0f;
		if (t <= times.front()) return values.front();

		for (size_t i = 1; i < times.size(); i++)
		{
			if (t <= times[i])
			{
				const float span = times[i] - times[i - 1];
				const float rate = (span > 0.0f) ? (t - times[i - 1]) / span : 0.0f;
				return values[i - 1] + (values[i] - values[i - 1]) * rate;
			}
		}
		return values.back();
	}

	// XZ：区間の中で水平に進んだ距離
	float MeasureXZ(const RootMotionCurve& curve, float start, float end)
	{
		return SampleCurve(curve.times, curve.xz, end) - SampleCurve(curve.times, curve.xz, start);
	}

	// Y：区間の中での最高点
	float MeasureYPeak(const RootMotionCurve& curve, float start, float end)
	{
		const float base = SampleCurve(curve.times, curve.y, start);
		float peak = SampleCurve(curve.times, curve.y, end) - base;

		for (size_t i = 0; i < curve.times.size(); i++)
		{
			if (curve.times[i] <= start || curve.times[i] >= end) continue;
			const float d = curve.y[i] - base;
			if (std::fabs(d) > std::fabs(peak)) peak = d;
		}
		return peak;
	}

	// 区間の並びを、タイムラインの時刻の範囲の並びにする
	std::vector<StudioTimelineWidget::TimeSpan> ToSpans(const std::vector<AnimationEditSection>& sections, const Model::Animation& animation)
	{
		std::vector<StudioTimelineWidget::TimeSpan> spans;
		spans.reserve(sections.size());
		for (const auto& s : sections)
		{
			// 生データの時刻 → タイムラインの時刻
			spans.push_back({ animation.ToBakedTime(s.start), animation.ToBakedTime(s.end) });
		}
		return spans;
	}
}

bool StudioAnimationEdit::DrawRootMotionTracks(const Model& model, const Model::Animation& animation, const std::string& clipName, float frameStep)
{
	// クリップが変わったら、区間の選択を外す
	if (curveClipName != clipName)
	{
		selectedSection = -1;
	}

	// 必要なときだけ作り直す
	if (curveDirty || curveClipName != clipName)
	{
		RebuildCurves(model, clipName);
	}

	bool changed = false;
	float tintStart = -1.0f;
	float tintEnd = -1.0f;

	// 表示している軸だけ表示
	const bool isY = (viewAxis == SectionAxis::Y);
	const ImU32 color = isY ? IM_COL32(255, 183, 77, 255) : IM_COL32(79, 195, 247, 255);

	GetSelectedSectionRange(viewAxis, animation, clipName, tintStart, tintEnd);
	const float graphTop = DrawCurveTrack(isY ? "Y" : "XZ", isY, 160.0f, color, tintStart, tintEnd);
	changed |= DrawSectionBand(isY ? "##SectionsY" : "##SectionsXZ", viewAxis, animation, clipName, frameStep, graphTop);

	return changed;
}

// グラフの値を作り直す
void StudioAnimationEdit::RebuildCurves(const Model& model, const std::string& clipName)
{
	originalCurve = RootMotionCurve{};
	editedCurve = RootMotionCurve{};
	curveClipName = clipName;
	curveDirty = false;

	const int rootIndex = AnimationEditBaker::FindNodeIndex(model.GetNodes(), editData.rootNodeName);
	if (rootIndex < 0) return;

	// 編集後：焼き込み後のクリップ
	if (const Model::Animation* edited = model.FindAnimation(clipName))
	{
		editedCurve = BuildCurve(*edited, rootIndex);
	}

	// 元：同じ設定から、区間だけを外して焼いたもの
	if (const Model::Animation* raw = model.FindRawAnimation(clipName))
	{
		const AnimationEditClipData* existing = editData.FindClip(clipName);
		AnimationEditClipData withoutSections = existing ? *existing : AnimationEditClipData{};
		withoutSections.sectionsXZ.clear();
		withoutSections.sectionsY.clear();

		const Model::Animation baked = AnimationEditBaker::BakeClip(*raw, model.GetNodes(), rootIndex, &withoutSections);
		originalCurve = BuildCurve(baked, rootIndex);
	}
}

// グラフのトラックを1本描く
float StudioAnimationEdit::DrawCurveTrack(const char* label, bool isY, float height, ImU32 color, float tintStart, float tintEnd)
{
	const std::vector<float>& original = isY ? originalCurve.y : originalCurve.xz;
	const std::vector<float>& edited = isY ? editedCurve.y : editedCurve.xz;

	// 縦の範囲
	float minValue = 0.0f;
	float maxValue = 0.0f;
	for (float v : original) { minValue = (std::min)(minValue, v); maxValue = (std::max)(maxValue, v); }
	for (float v : edited) { minValue = (std::min)(minValue, v); maxValue = (std::max)(maxValue, v); }

	// ほとんど動かないときは、線が真ん中に来るように範囲を広げる
	if (maxValue - minValue < 0.1f)
	{
		const float center = (minValue + maxValue) * 0.5f;
		minValue = center - 0.05f;
		maxValue = center + 0.05f;
	}

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const float margin = 4.0f;
	const float padding = 6.0f; // 線が枠に張り付かないための余白
	const float top = ImGui::GetCursorScreenPos().y + margin;
	const float bottom = top + height;

	// 背景
	drawList->AddRectFilled(ImVec2(timeline.GetLeft(), top), ImVec2(timeline.GetRight(), bottom), IM_COL32(43, 45, 49, 255), 3.0f);

	// 画面のY座標
	const float usableHeight = height - padding * 2.0f;
	auto toY = [&](float value)
		{
			const float rate = (value - minValue) / (maxValue - minValue);
			return bottom - padding - rate * usableHeight;
		};

	// 横線の目盛り
	const float candidates[] = { 0.01f, 0.02f, 0.05f, 0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f };
	float interval = candidates[9];
	for (float c : candidates)
	{
		if (usableHeight * c / (maxValue - minValue) >= 24.0f)
		{
			interval = c;
			break;
		}
	}

	// 選んでいる区間の範囲を薄く色付けする
	if (tintEnd > tintStart)
	{
		const float x0 = (std::max)(timeline.TimeToX(tintStart), timeline.GetLeft());
		const float x1 = (std::min)(timeline.TimeToX(tintEnd), timeline.GetRight());
		if (x1 > x0)
		{
			const ImU32 tintColor = (color & 0x00FFFFFF) | (40u << 24); // 線の色を薄くしたもの
			drawList->AddRectFilled(ImVec2(x0, top), ImVec2(x1, bottom), tintColor);
		}
	}

	// 間隔に合わせて小数の桁を決める
	const char* gridFormat = (interval < 0.1f) ? "%.2f" : (interval < 1.0f) ? "%.1f" : "%.0f";

	const int firstLine = (int)std::ceil(minValue / interval - 0.0001f);
	const int lastLine = (int)std::floor(maxValue / interval + 0.0001f);
	for (int i = firstLine; i <= lastLine; i++)
	{
		const float value = i * interval;
		const float y = toY(value);

		// 0m の線は少し明るくして、基準にする
		const ImU32 lineColor = (i == 0) ? IM_COL32(90, 92, 98, 255) : IM_COL32(60, 62, 68, 255);
		drawList->AddLine(ImVec2(timeline.GetLeft(), y), ImVec2(timeline.GetRight(), y), lineColor, 1.0f);

		// 値のラベル（右端、線の上。上にはみ出すときは線の下）
		char gridLabel[16];
		snprintf(gridLabel, sizeof(gridLabel), gridFormat, value);
		const ImVec2 textSize = ImGui::CalcTextSize(gridLabel);
		const float labelY = (y - textSize.y - 1.0f < top) ? (y + 1.0f) : (y - textSize.y - 1.0f);
		drawList->AddText(ImVec2(timeline.GetRight() - textSize.x - 4.0f, labelY), IM_COL32(130, 130, 130, 255), gridLabel);
	}

	// 画面の点
	auto toPoints = [&](const std::vector<float>& times, const std::vector<float>& values)
		{
			std::vector<ImVec2> points;
			points.reserve(values.size());
			for (size_t i = 0; i < values.size(); i++)
			{
				points.push_back(ImVec2(timeline.TimeToX(times[i]), toY(values[i])));
			}
			return points;
		};

	// 元（点線）を先に、編集後（実線）を上に重ねる
	const std::vector<ImVec2> originalPoints = toPoints(originalCurve.times, original);
	const std::vector<ImVec2> editedPoints = toPoints(editedCurve.times, edited);
	AddDashedPolyline(drawList, originalPoints, IM_COL32(140, 140, 140, 255), 1.5f);
	if (editedPoints.size() >= 2)
	{
		drawList->AddPolyline(editedPoints.data(), (int)editedPoints.size(), color, 0, 2.0f);
	}

	// ラベル：トラック名と、最後の値
	char text[32];
	snprintf(text, sizeof(text), "%s  %.2f m", label, edited.empty() ? 0.0f : edited.back());
	drawList->AddText(ImVec2(timeline.GetLeft() + 4.0f, top + 2.0f), IM_COL32(200, 200, 200, 255), text);

	// 描いた分の高さを確保する
	ImGui::Dummy(ImVec2(0.0f, height + margin * 2.0f));

	return top;
}

// 区間の帯を描いて、クリックで選択する
bool StudioAnimationEdit::DrawSectionBand(const char* id, SectionAxis axis, const Model::Animation& animation, const std::string& clipName, float frameStep, float graphTop)
{
	const float length = animation.secondsLength;
	const AnimationEditClipData* clip = editData.FindClip(clipName);
	const std::vector<AnimationEditSection>* sections = nullptr;
	if (clip) sections = (axis == SectionAxis::XZ) ? &clip->sectionsXZ : &clip->sectionsY;

	const ImU32 color = (axis == SectionAxis::XZ) ? IM_COL32(79, 195, 247, 255) : IM_COL32(255, 183, 77, 255);
	const ImU32 dimColor = (axis == SectionAxis::XZ) ? IM_COL32(79, 195, 247, 110) : IM_COL32(255, 183, 77, 110);

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const float margin = 2.0f;
	const float bandHeight = 22.0f;
	const ImVec2 cursor = ImGui::GetCursorScreenPos();
	const float top = cursor.y + margin;
	const float bottom = top + bandHeight;

	// 帯の全体をクリックできるようにする
	ImGui::SetCursorScreenPos(ImVec2(timeline.GetLeft(), top));
	const bool clicked = ImGui::InvisibleButton(id, ImVec2(timeline.GetRight() - timeline.GetLeft(), bandHeight));
	const float mouseX = ImGui::GetIO().MousePos.x;

	// 背景
	drawList->AddRectFilled(ImVec2(timeline.GetLeft(), top), ImVec2(timeline.GetRight(), bottom), IM_COL32(50, 52, 56, 255), 3.0f);

	int hitIndex = -1;
	if (sections)
	{
		for (int i = 0; i < (int)sections->size(); i++)
		{
			const AnimationEditSection& s = (*sections)[i];

			// 生データの時刻 → タイムラインの時刻
			const float start = animation.ToBakedTime(s.start);
			const float end = animation.ToBakedTime(s.end);

			// カットの範囲外にはみ出した部分は描かない
			const float x0 = (std::max)(timeline.TimeToX(start), timeline.GetLeft());
			const float x1 = (std::min)(timeline.TimeToX(end), timeline.GetRight());
			if (x1 <= x0) continue;

			const bool selected = (selectedAxis == axis && selectedSection == i);
			drawList->AddRectFilled(ImVec2(x0, top + 2.0f), ImVec2(x1, bottom - 2.0f), selected ? color : dimColor, 3.0f);

			// 距離のラベル
			const float value = (axis == SectionAxis::XZ) ? MeasureXZ(editedCurve, start, end) : MeasureYPeak(editedCurve, start, end);
			char label[32];
			snprintf(label, sizeof(label), "%.2f m", value);
			const ImVec2 textSize = ImGui::CalcTextSize(label);
			if (textSize.x + 6.0f < x1 - x0)
			{
				drawList->AddText(
					ImVec2((x0 + x1 - textSize.x) * 0.5f, (top + bottom - textSize.y) * 0.5f),
					selected ? IM_COL32(0, 0, 0, 255) : IM_COL32(220, 220, 220, 255),
					label);
			}

			if (mouseX >= x0 && mouseX <= x1) hitIndex = i;
		}
	}

	// クリックした所の区間を選ぶ
	if (clicked)
	{
		selectedAxis = axis;
		selectedSection = hitIndex;
	}

	// 選んでいる区間の端のピン
	bool changed = false;
	if (sections && selectedAxis == axis && selectedSection >= 0 && selectedSection < (int)sections->size())
	{
		const AnimationEditSection& s = (*sections)[selectedSection];
		float start = animation.ToBakedTime(s.start);
		float end = animation.ToBakedTime(s.end);

		// 隣の区間にぶつかったら止まる
		float lower = 0.0f;
		float upper = length;
		GetSectionLimits(*sections, selectedSection, animation, lower, upper);

		const float minLength = StudioTimelineWidget::MinLength(frameStep);
		const std::string handleId = std::string(id) + "Handles";
		if (timeline.DrawRangeHandles(handleId.c_str(), bottom, frameStep, minLength, start, end, lower, upper, graphTop))
		{
			// 生データの時刻に戻して書き込む
			AnimationEditClipData* editable = editData.FindOrCreateClip(clipName);
			auto& list = (axis == SectionAxis::XZ) ? editable->sectionsXZ : editable->sectionsY;
			list[selectedSection].start = animation.ToRawTime(start);
			list[selectedSection].end = animation.ToRawTime(end);
			changed = true;
		}
	}

	// 帯とピンの頭の分の高さを確保する
	ImGui::SetCursorScreenPos(ImVec2(cursor.x, bottom + StudioTimelineWidget::RangeHandleHeight + margin));
	ImGui::Dummy(ImVec2(0.0f, 0.0f));

	return changed;
}

// 区間の追加とSet Start / Set End
bool StudioAnimationEdit::DrawSectionButtons(const Model::Animation& animation, const std::string& clipName, float frameStep, float pinTime)
{
	const float length = animation.secondsLength;
	const float minLength = StudioTimelineWidget::MinLength(frameStep);
	const float tolerance = minLength * 0.5f; // 小数の誤差として許す量

	const AnimationEditClipData* clip = editData.FindClip(clipName);
	bool changed = false;


	// 区間を追加して、作った区間を選ぶ
	auto addSection = [&](SectionAxis axis, float end)
		{
			AnimationEditClipData* editable = editData.FindOrCreateClip(clipName);
			auto& list = (axis == SectionAxis::XZ) ? editable->sectionsXZ : editable->sectionsY;

			AnimationEditSection section;
			section.start = animation.ToRawTime(pinTime); // 生データの時刻で保存する
			section.end = animation.ToRawTime(end);
			section.scale = 1.0f;

			// 開始時刻の順になるように差し込む
			auto it = std::find_if(list.begin(), list.end(),
				[&](const AnimationEditSection& s) { return s.start > section.start; });
			const int index = (int)(it - list.begin());
			list.insert(it, section);

			selectedAxis = axis;
			selectedSection = index;
			changed = true;
		};

	// 追加
	{
		const std::vector<AnimationEditSection>* list = nullptr;
		if (clip) list = (viewAxis == SectionAxis::XZ) ? &clip->sectionsXZ : &clip->sectionsY;

		float end = 0.0f;
		const bool enabled = StudioTimelineWidget::FindAddEnd(
			list ? ToSpans(*list, animation) : std::vector<StudioTimelineWidget::TimeSpan>{}, 
			pinTime, length, minLength, end);

		ImGui::BeginDisabled(!enabled);
		if (ImGui::Button("Add Section"))
		{
			addSection(viewAxis, end);
			clip = editData.FindClip(clipName); // 追加で中身が変わったので取り直す
		}
		ImGui::EndDisabled();
	}

	// 選んでいる区間の SetStart/SetEnd
	const std::vector<AnimationEditSection>* selectedList = nullptr;
	if (clip) selectedList = (selectedAxis == SectionAxis::XZ) ? &clip->sectionsXZ : &clip->sectionsY;
	const bool hasSelection = selectedList && selectedSection >= 0 && selectedSection < (int)selectedList->size();

	float start = 0.0f;
	float end = 0.0f;
	float lower = 0.0f;
	float upper = length;
	if (hasSelection)
	{
		start = animation.ToBakedTime((*selectedList)[selectedSection].start);
		end = animation.ToBakedTime((*selectedList)[selectedSection].end);
		GetSectionLimits(*selectedList, selectedSection, animation, lower, upper);
	}

	ImGui::SameLine();
	ImGui::Dummy(ImVec2(16.0f, 0.0f)); // 追加ボタンとの間を空ける
	ImGui::SameLine();

	// Set Start：前の区間に食い込まず、終わりとの間を1フレーム残す
	ImGui::BeginDisabled(!hasSelection || pinTime < lower - tolerance || pinTime > end - minLength + tolerance);
	if (ImGui::Button("Set Start##Section"))
	{
		AnimationEditClipData* editable = editData.FindOrCreateClip(clipName);
		auto& list = (selectedAxis == SectionAxis::XZ) ? editable->sectionsXZ : editable->sectionsY;
		list[selectedSection].start = animation.ToRawTime(pinTime);
		changed = true;
	}
	ImGui::EndDisabled();

	ImGui::SameLine();

	// Set End：後ろの区間に食い込まず、始まりとの間を1フレーム残す
	ImGui::BeginDisabled(!hasSelection || pinTime > upper + tolerance || pinTime < start + minLength - tolerance);
	if (ImGui::Button("Set End##Section"))
	{
		AnimationEditClipData* editable = editData.FindOrCreateClip(clipName);
		auto& list = (selectedAxis == SectionAxis::XZ) ? editable->sectionsXZ : editable->sectionsY;
		list[selectedSection].end = animation.ToRawTime(pinTime);
		changed = true;
	}
	ImGui::EndDisabled();

	return changed;
}

// 選んでいる区間の数値
bool StudioAnimationEdit::DrawSectionSettings(const Model& model, AnimationEditClipData& clip)
{
	const Model::Animation* animation = model.FindAnimation(clip.clipName);
	auto& list = (selectedAxis == SectionAxis::XZ) ? clip.sectionsXZ : clip.sectionsY;
	if (!animation || selectedAxis != viewAxis || selectedSection < 0 || selectedSection >= (int)list.size())
	{
		ImGui::TextDisabled("No section selected.");
		return false;
	}

	AnimationEditSection& section = list[selectedSection];
	const bool isY = (selectedAxis == SectionAxis::Y);

	// 範囲（生データの時刻 → タイムラインの時刻）
	const float start = animation->ToBakedTime(section.start);
	const float end = animation->ToBakedTime(section.end);
	const float frameStep = CalcFrameStep(model, clip.clipName);

	ImGui::Text("Range     %.3f - %.3f s", start, end);
	ImGui::TextDisabled("          F%d - F%d", StudioTimelineWidget::ToFrame(start, frameStep), StudioTimelineWidget::ToFrame(end, frameStep));

	// 元の値
	const float original = isY ? MeasureYPeak(originalCurve, start, end) : MeasureXZ(originalCurve, start, end);
	ImGui::Text(isY ? "Original peak  %.2f m" : "Original  %.2f m", original);

	bool changed = false;

	// 元がほぼ0なら、倍率が計算できないので入力できない
	const bool canEdit = std::fabs(original) > 0.001f;

	// 入力は「距離」、保存は「倍率」（距離 = 元の値 × 倍率）
	float target = original * section.scale;

	// 元と逆向きの値は入れられないようにする
	const float minValue = (original >= 0.0f) ? 0.0f : -100.0f;
	const float maxValue = (original >= 0.0f) ? 100.0f : 0.0f;

	ImGui::BeginDisabled(!canEdit);
	if (ImGui::DragFloat(isY ? "Peak" : "Distance", &target, 0.01f, minValue, maxValue, "%.2f m"))
	{
		section.scale = (std::max)(target / original, 0.0f);
		changed = true;
	}
	ImGui::EndDisabled();

	if (canEdit)
	{
		ImGui::TextDisabled("          x%.2f", section.scale);
	}
	else
	{
		ImGui::TextDisabled("No motion in this section.");
	}

	// 削除
	if (ImGui::Button("Delete Section"))
	{
		list.erase(list.begin() + selectedSection);
		selectedSection = -1;
		changed = true;
	}

	return changed;
}

// 選んでいる区間の範囲
bool StudioAnimationEdit::GetSelectedSectionRange(SectionAxis axis, const Model::Animation& animation, const std::string& clipName, float& outStart, float& outEnd) const
{
	outStart = -1.0f;
	outEnd = -1.0f;
	if (selectedAxis != axis || selectedSection < 0) return false;

	const AnimationEditClipData* clip = editData.FindClip(clipName);
	if (!clip) return false;

	const auto& sections = (axis == SectionAxis::XZ) ? clip->sectionsXZ : clip->sectionsY;
	if (selectedSection >= (int)sections.size()) return false;

	outStart = animation.ToBakedTime(sections[selectedSection].start);
	outEnd = animation.ToBakedTime(sections[selectedSection].end);

	return true;
}

// 区間が動ける範囲
void StudioAnimationEdit::GetSectionLimits(const std::vector<AnimationEditSection>& sections, int index, const Model::Animation& animation, float& outLower, float& outUpper)
{
	StudioTimelineWidget::GetSpanLimits(ToSpans(sections, animation), index, animation.secondsLength, outLower, outUpper);
}