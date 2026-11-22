#include "StudioAnimationEdit.h"
#include "Engine/ECS/Components/SkinnedMeshComponent.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/Resource/AnimationEditBaker.h"
#include <algorithm>
#include <cstdio>
#include <cmath>


// ImGui描画
void StudioAnimationEdit::DrawGUI(entt::registry& registry, StudioContext& context)
{
	if (!context.showAnimationEdit) return;

	std::string panelName = "Animation Edit##" + std::to_string(windowId);
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

	// モデルが切り替わったら、そのモデル用の設定を読み込み直す
	if (loadedModelPath != context.currentModelPath)
	{
		Load(context.currentModelPath, *skinned.model);
	}

	if (animator.currentAnimation.empty())
	{
		ImGui::TextDisabled("Select an animation from the Animator panel.");
		ImGui::End();
		return;
	}

	// モードを決めてから、焼き方を合わせる
	DrawModeBar();
	SyncBakeWithMode(*skinned.model, animator, context);

	const std::string& clipName = animator.currentAnimation;
	const float detailWidth = 260.0f;

	// 左：タイムライン
	ImGui::BeginChild("AnimEditTimeline", ImVec2(-detailWidth, 0.0f));
	bool changed = DrawTimelineArea(*skinned.model, animator, context);
	ImGui::EndChild();

	ImGui::SameLine();

	// 右：数値
	ImGui::BeginChild("AnimEditDetail", ImVec2(detailWidth, 0.0f), true);
	changed |= DrawDetailArea(*skinned.model, clipName, context.currentModelPath);
	ImGui::EndChild();

	// どちらかで値が変わったら、生データから焼き直す
	if (changed)
	{
		Rebake(*skinned.model);
		context.requestPoseUpdate = true;
		isDirty = true;
		curveDirty = true;
	}

	ImGui::End();
}

// ウィンドウを閉じている間に呼ぶ
void StudioAnimationEdit::OnHidden(entt::registry& registry, StudioContext& context)
{
	if (!previewActive) return;

	auto view = registry.view<SkinnedMeshComponent, AnimatorComponent>();
	if (view.begin() == view.end())
	{
		previewActive = false;
		return;
	}

	auto entity = *view.begin();
	auto& skinned = view.get<SkinnedMeshComponent>(entity);
	auto& animator = view.get<AnimatorComponent>(entity);

	if (!skinned.model)
	{
		previewActive = false;
		return;
	}

	SetPreviewActive(*skinned.model, animator, context, false);
}

// モードの切り替え
void StudioAnimationEdit::DrawModeBar()
{
	if (ImGui::RadioButton("Trim", mode == EditMode::Trim)) mode = EditMode::Trim;
	ImGui::SameLine();
	if (ImGui::RadioButton("Root Motion", mode == EditMode::RootMotion)) mode = EditMode::RootMotion;

	// ルートモーションモードでは、表示する軸を切り替える
	if (mode == EditMode::RootMotion)
	{
		auto setViewAxis = [&](SectionAxis axis)
			{
				viewAxis = axis;
				// 見えなくなる軸の区間を選んでいたら、選択を外す
				if (selectedAxis != viewAxis) selectedSection = -1;
			};

		ImGui::SameLine();
		ImGui::TextDisabled("|");
		ImGui::SameLine();
		if (ImGui::RadioButton("XZ##ViewAxis", viewAxis == SectionAxis::XZ)) setViewAxis(SectionAxis::XZ);
		ImGui::SameLine();
		if (ImGui::RadioButton("Y##ViewAxis", viewAxis == SectionAxis::Y)) setViewAxis(SectionAxis::Y);
	}
	ImGui::Separator();
}
// タイムライン
bool StudioAnimationEdit::DrawTimelineArea(const Model& model, AnimatorComponent& animator, StudioContext& context)
{
	const std::string& clipName = animator.currentAnimation;

	// クリップの長さ
	const Model::Animation* animation = previewActive ? model.FindPreviewAnimation(clipName) : model.FindAnimation(clipName);
	const float length = animation ? animation->secondsLength : 0.0f;

	const float frameStep = CalcFrameStep(model, clipName);

	// 再生ボタンと現在時刻
	if (mode == EditMode::Trim)
	{
		// トリミングモード：使う範囲だけを、このツールが時刻を進めて再生する
		float trimStart = 0.0f;
		float trimEnd = 0.0f;
		GetTrimRange(clipName, length, trimStart, trimEnd);

		if (StudioTimelineWidget::DrawToolbar(rangePlaying, animator.animationTime, frameStep))
		{
			if (rangePlaying)
			{
				// 範囲の外から再生したら、Start から
				if (animator.animationTime < trimStart || animator.animationTime >= trimEnd)
				{
					animator.animationTime = trimStart;
				}
				animator.isPlaying = false; // Animator には進めさせない
				animator.isEnd = false;
				context.requestPoseUpdate = true;
			}
		}

		if (rangePlaying)
		{
			AdvanceRangePlayback(animator, context, trimStart, trimEnd);
		}
	}
	else
	{
		// ルートモーションモード：Animator に再生させる
		StudioTimelineWidget::DrawAnimatorToolbar(animator, length, frameStep);
	}

	// タイムライン
	if (timeline.BeginWithAnimator("##AnimEditRuler", length, frameStep, animator, context))
	{
		rangePlaying = false;
	}

	// トラック
	bool changed = false;
	if (mode == EditMode::Trim)
	{
		changed |= DrawTrimTrack(clipName, length, frameStep);
	}
	else if (animation)
	{
		changed |= DrawRootMotionTracks(model, *animation, clipName, frameStep);
	}

	timeline.End();

	// ピンの位置で操作するボタン
	const float pinTime = StudioTimelineWidget::SnapToFrame(animator.animationTime, frameStep, length);
	if (mode == EditMode::Trim)
	{
		changed |= DrawTrimButtons(clipName, length, frameStep, pinTime);
	}
	else if (animation)
	{
		changed |= DrawSectionButtons(*animation, clipName, frameStep, pinTime);
	}

	return changed;
}

// 数値
bool StudioAnimationEdit::DrawDetailArea(const Model& model, const std::string& clipName, const std::string& modelPath)
{
	// モデル全体の設定
	bool changed = DrawRootNodeCombo(model);
	ImGui::Separator();

	// クリップごとの設定
	ImGui::Text("Clip: %s", clipName.c_str());
	changed |= DrawClipSettings(model, clipName);

	ImGui::Separator();

	// リセット：このクリップの設定を消す
	if (editData.FindClip(clipName) && ImGui::Button("Reset Clip"))
	{
		auto& clips = editData.clips;
		clips.erase(
			std::remove_if(clips.begin(), clips.end(),
				[&](const AnimationEditClipData& c) { return c.clipName == clipName; }),
			clips.end());
		changed = true;
	}

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

// Root Node の選択
bool StudioAnimationEdit::DrawRootNodeCombo(const Model& model)
{
	bool changed = false;

	if (ImGui::BeginCombo("Root Node", editData.rootNodeName.c_str()))
	{
		// 一覧と自動の値は、開いている間だけ調べる
		const auto& nodes = model.GetNodes();
		const std::string defaultName = AnimationEditBaker::FindDefaultRootNodeName(model);

		for (int index : AnimationEditBaker::CollectBoneIndices(model))
		{
			const std::string& name = nodes[index].name;
			const bool selected = (name == editData.rootNodeName);

			// 自動で見つけたボーンには目印を付ける
			const std::string label = (name == defaultName) ? name + "  (auto)" : name;

			if (ImGui::Selectable(label.c_str(), selected) && !selected)
			{
				editData.rootNodeName = name;
				changed = true;
			}
			if (selected) ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}

	return changed;
}

// クリップの設定
bool StudioAnimationEdit::DrawClipSettings(const Model& model, const std::string& clipName)
{
	// 設定が無いクリップは既定値で表示する
	const AnimationEditClipData* existing = editData.FindClip(clipName);
	AnimationEditClipData clip = existing ? *existing : AnimationEditClipData{};
	clip.clipName = clipName;

	bool changed = false;

	// 向きの補正
	ImGui::Separator();
	ImGui::TextDisabled("Orientation");
	changed |= ImGui::DragFloat("Yaw Offset", &clip.yawOffsetDegrees, 1.0f, -180.0f, 180.0f, "%.1f deg");

	// モードごとの項目
	ImGui::Separator();
	if (mode == EditMode::Trim)
	{
		ImGui::TextDisabled("Trimming");
		changed |= DrawTrimSettings(model, clip);
	}
	else
	{
		ImGui::TextDisabled((viewAxis == SectionAxis::XZ) ? "Section (XZ)" : "Section (Y)");
		changed |= DrawSectionSettings(model, clip);
	}

	// 変更があったときだけ、データに書き戻す
	if (changed)
	{
		*editData.FindOrCreateClip(clipName) = clip;
	}

	return changed;
}

// 1フレームの秒数
float StudioAnimationEdit::CalcFrameStep(const Model& model, const std::string& clipName)
{
	const Model::Animation* raw = model.FindRawAnimation(clipName);
	return raw ? raw->CalcFrameStep() : 0.0f;
}
// 生データの長さ
float StudioAnimationEdit::FindRawLength(const Model& model, const std::string& clipName)
{
	const Model::Animation* raw = model.FindRawAnimation(clipName);
	return raw ? raw->secondsLength : 0.0f;
}

// モードに合わせて焼き直す
void StudioAnimationEdit::Rebake(Model& model) const
{
	// 編集済み
	model.ApplyAnimationEdit(editData);

	// トリミングのプレビュー中は、プレビュー用も焼き直す
	if (previewActive)
	{
		model.ApplyAnimationEditPreview(editData);
	}
}
// モードと焼き方がずれていたら合わせる
void StudioAnimationEdit::SyncBakeWithMode(Model& model, AnimatorComponent& animator, StudioContext& context)
{
	SetPreviewActive(model, animator, context, mode == EditMode::Trim);
}
// トリミングのプレビューを使う／やめる
void StudioAnimationEdit::SetPreviewActive(Model& model, AnimatorComponent& animator, StudioContext& context, bool active)
{
	// Animator の切り替えは毎回合わせる
	animator.usePreview = active;
	if (active == previewActive) return;

	// 編集済みのクリップの時刻の対応表を使って、同じポーズを指すように時刻を直す
	const Model::Animation* edited = model.FindAnimation(animator.currentAnimation);

	if (active)
	{
		model.ApplyAnimationEditPreview(editData);

		// カット後の時刻 → カット前の時刻
		if (edited) animator.animationTime = edited->ToRawTime(animator.animationTime);
	}
	else
	{
		// カット前の時刻 → カット後の時刻（カットされた部分なら、端に寄せる）
		if (edited) animator.animationTime = edited->ToBakedTime(animator.animationTime);

		model.ClearAnimationPreview();
	}

	previewActive = active;
	rangePlaying = false; // 範囲の再生は止める
	curveDirty = true;    // グラフも作り直す
	context.requestPoseUpdate = true;
}

// 読み込み
void StudioAnimationEdit::Load(const std::string& modelPath, const Model& model)
{
	// 無い・壊れているときは初期値が返る。無くて普通のファイルなので、ログは出さない
	editData = fileCache.GetOrLoad(AnimationEditData::GetFilePath(modelPath),
		[](AnimationEditData& data, const nlohmann::json& j) { data.FromJson(j); }, JsonMissingLog::Silent);

	loadedModelPath = modelPath;
	isDirty = false;
	previewActive = false; // 読み込んだばかりのモデルには、プレビュー用はまだ無い
	curveDirty = true;
	selectedSection = -1;

	// ルートノードが未設定なら、モデルから探す
	if (editData.rootNodeName.empty())
	{
		editData.rootNodeName = AnimationEditBaker::FindDefaultRootNodeName(model);
	}
}
void StudioAnimationEdit::Save(const std::string& modelPath)
{
	if (modelPath.empty()) return;

	if (fileCache.Save(AnimationEditData::GetFilePath(modelPath), editData,
		[](const AnimationEditData& data) { return data.ToJson(); }))
	{
		isDirty = false;
	}
}
