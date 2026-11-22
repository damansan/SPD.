#include "StudioAnimationSpeed.h"
#include "Engine/ECS/Components/SkinnedMeshComponent.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/ECS/Components/AnimationSpeedComponent.h"

#include <algorithm>
#include <fstream>
#include <filesystem>

// ImGui描画
void StudioAnimationSpeed::DrawGUI(entt::registry& registry, StudioContext& context)
{
	if (!context.showAnimationSpeed) return;

	std::string panelName = "Animation Speed##" + std::to_string(windowId);
	ImGui::Begin(panelName.c_str());

	RefreshAvailableAnimations(registry);

	auto view = registry.view<SkinnedMeshComponent, AnimatorComponent, AnimationSpeedComponent>();
	if (view.begin() == view.end())
	{
		ImGui::Text("No skinned model.");
		ImGui::End();
		return;
	}

	auto entity = *view.begin();
	auto& skinned = view.get<SkinnedMeshComponent>(entity);
	auto& animator = view.get<AnimatorComponent>(entity);
	auto& animSpeed = view.get<AnimationSpeedComponent>(entity);

	if (!skinned.model)
	{
		ImGui::End();
		return;
	}

	// モデルが切り替わったら、そのモデル用の速度カーブデータを読み込み直す
	if (loadedModelPath != context.currentModelPath)
	{
		animationSpeedData = AnimationSpeedData();

		std::string loadPath = GetAnimationSpeedFilePath(context.currentModelPath);
		std::ifstream ifs(loadPath);
		if (ifs.is_open())
		{
			try
			{
				nlohmann::json j;
				ifs >> j;
				animationSpeedData.FromJson(j);
			}
			catch (const std::exception&) {}
		}

		// AnimationSpeedSystemが実際にこのデータを見つけられるよう、
		// このモデル用のパスをコンポーネントへ反映する(Sequencerには無い手順)
		animSpeed.animationSpeedPath = loadPath;

		loadedModelPath = context.currentModelPath;
		selectedPointId = -1;
	}

	if (animator.currentAnimation.empty())
	{
		ImGui::TextDisabled("Select an animation from the Animator panel.");
		ImGui::End();
		return;
	}

	// 選んでいるクリップが切り替わったら、選択状態をリセットする
	if (selectedClipName != animator.currentAnimation)
	{
		selectedClipName = animator.currentAnimation;
		selectedPointId = -1;
	}

	// このクリップの長さを取得する
	float clipLength = 0.0f;
	for (const auto& anim : skinned.model->GetAnimations())
	{
		if (anim.name == selectedClipName)
		{
			clipLength = anim.secondsLength;
			break;
		}
	}

	// このクリップが今再生中なら、プレビュー時刻として使う
	float previewTime = (animator.currentAnimation == selectedClipName) ? animator.animationTime : 0.0f;

	AnimationSpeedClipData* clip = animationSpeedData.FindOrCreateClip(selectedClipName);

	const float detailPanelWidth = 220.0f;

	ImGui::BeginChild("AnimationSpeedGraphArea", ImVec2(-detailPanelWidth, 0.0f));
	DrawToolbar(*clip);
	DrawSpeedGraph(*clip, previewTime, clipLength);
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("AnimationSpeedDetail", ImVec2(detailPanelWidth, 0.0f), true);
	DrawDetailPanel(*clip, context.currentModelPath);
	ImGui::EndChild();

	ImGui::End();
}

// ツールバー
void StudioAnimationSpeed::DrawToolbar(AnimationSpeedClipData& clip)
{
	ImGui::TextDisabled("Clip: %s", clip.clipName.c_str());
	ImGui::TextDisabled("Points: %d", (int)clip.points.size());
	ImGui::Separator();
}
// グラフ描画
void StudioAnimationSpeed::DrawSpeedGraph(AnimationSpeedClipData& clip, float previewTime, float clipLength)
{
	const float speedMin = 0.0f;
	const float speedMax = 3.0f;

	ImGui::Text("Speed Curve");
	ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60.0f);
	if (ImGui::Button("+ Add"))
	{
		AnimationSpeedPoint p;
		p.id = clip.GenerateId();
		p.time = TimeToNormalized(previewTime, clipLength);
		p.speed = 1.0f;
		p.isEndpoint = false;
		clip.points.push_back(p);

		selectedPointId = p.id;
	}

	ImVec2 graphMin = ImGui::GetCursorScreenPos();
	float graphWidth = ImGui::GetContentRegionAvail().x;
	const float graphHeight = 260.0f;
	ImVec2 graphMax(graphMin.x + graphWidth, graphMin.y + graphHeight);

	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->AddRectFilled(graphMin, graphMax, IM_COL32(30, 30, 30, 255));

	auto timeToX = [&](float normalized) { return graphMin.x + normalized * graphWidth; };
	auto speedToY = [&](float speed)
		{
			float t = (std::clamp(speed, speedMin, speedMax) - speedMin) / (speedMax - speedMin);
			return graphMax.y - t * graphHeight; // 上に行くほど速い
		};
	auto xToTime = [&](float screenX) { return std::clamp((screenX - graphMin.x) / graphWidth, 0.0f, 1.0f); };
	auto yToSpeed = [&](float screenY)
		{
			float t = std::clamp((graphMax.y - screenY) / graphHeight, 0.0f, 1.0f);
			return speedMin + t * (speedMax - speedMin);
		};

	// 横方向の目盛り線(0.5刻み。1.0倍だけ強調)
	for (float s = speedMin; s <= speedMax + 0.001f; s += 0.5f)
	{
		float y = speedToY(s);
		bool isNormal = (std::abs(s - 1.0f) < 0.001f);
		ImU32 lineColor = isNormal ? IM_COL32(120, 120, 60, 180) : IM_COL32(60, 60, 60, 150);
		dl->AddLine(ImVec2(graphMin.x, y), ImVec2(graphMax.x, y), lineColor, isNormal ? 1.5f : 1.0f);

		char label[16];
		snprintf(label, sizeof(label), "%.1fx", s);
		dl->AddText(ImVec2(graphMin.x + 4.0f, y - 14.0f), IM_COL32(160, 160, 160, 255), label);
	}

	// プレイヘッド(現在の再生位置)
	float phX = timeToX(TimeToNormalized(previewTime, clipLength));
	dl->AddLine(ImVec2(phX, graphMin.y), ImVec2(phX, graphMax.y), IM_COL32(220, 60, 60, 220), 1.5f);

	// ポイントをtime順に並べ替えて、曲線として繋いで描画する
	std::vector<AnimationSpeedPoint*> sorted;
	sorted.reserve(clip.points.size());
	for (auto& p : clip.points) sorted.push_back(&p);
	std::sort(sorted.begin(), sorted.end(),
		[](const AnimationSpeedPoint* a, const AnimationSpeedPoint* b) { return a->time < b->time; });

	for (size_t i = 0; i + 1 < sorted.size(); ++i)
	{
		ImVec2 a(timeToX(sorted[i]->time), speedToY(sorted[i]->speed));
		ImVec2 b(timeToX(sorted[i + 1]->time), speedToY(sorted[i + 1]->speed));
		dl->AddLine(a, b, IM_COL32(120, 170, 255, 255), 2.0f);
	}

	// ポイント本体(ドラッグ操作)
	ImGui::PushID("SpeedPointGraph");
	for (auto& p : clip.points)
	{
		ImVec2 center(timeToX(p.time), speedToY(p.speed));

		bool isSelected = (selectedPointId == p.id);
		ImU32 color = isSelected ? IM_COL32(255, 200, 80, 255)
			: p.isEndpoint ? IM_COL32(200, 120, 255, 255)
			: IM_COL32(120, 170, 255, 255);

		dl->AddCircleFilled(center, 5.0f, color);

		ImGui::SetCursorScreenPos(ImVec2(center.x - 6.0f, center.y - 6.0f));
		ImGui::PushID(p.id);
		ImGui::InvisibleButton("point", ImVec2(12.0f, 12.0f));

		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			ImVec2 mousePos = ImGui::GetMousePos();

			// 始端・終端はX(時間)を固定し、Yのみ動かせる
			if (!p.isEndpoint)
			{
				p.time = xToTime(mousePos.x);
			}
			p.speed = yToSpeed(mousePos.y);
		}

		if (ImGui::IsItemClicked())
		{
			selectedPointId = p.id;
		}

		ImGui::PopID();
	}
	ImGui::PopID();

	ImGui::SetCursorScreenPos(ImVec2(graphMin.x, graphMax.y + 4.0f));
	ImGui::Dummy(ImVec2(0.0f, 0.0f));
}

// 詳細パネル
void StudioAnimationSpeed::DrawDetailPanel(AnimationSpeedClipData& clip, const std::string& modelPath)
{
	if (ImGui::Button("Save", ImVec2(-1.0f, 0.0f)))
	{
		Save(modelPath);
	}

	ImGui::Separator();

	if (selectedPointId < 0)
	{
		ImGui::TextDisabled("No selection.");
		return;
	}

	AnimationSpeedPoint* point = nullptr;
	for (auto& p : clip.points)
	{
		if (p.id == selectedPointId) { point = &p; break; }
	}

	if (!point)
	{
		selectedPointId = -1;
		ImGui::TextDisabled("No selection.");
		return;
	}

	ImGui::Text(point->isEndpoint ? "Point (Fixed)" : "Point");
	ImGui::Separator();

	float timePct = point->time * 100.0f;

	// 始端・終端は時間(X)を固定するため編集不可にする
	ImGui::BeginDisabled(point->isEndpoint);
	if (ImGui::DragFloat("Time %", &timePct, 0.1f, 0.0f, 100.0f))
	{
		point->time = timePct * 0.01f;
	}
	ImGui::EndDisabled();

	ImGui::DragFloat("Speed", &point->speed, 0.01f, 0.0f, 3.0f, "%.2fx");

	// 始端・終端は削除不可
	if (!point->isEndpoint)
	{
		int pointId = point->id;
		DrawDeleteButton("Delete Point", [this, &clip, pointId]() { DeletePoint(clip, pointId); });
	}
}

// ポイント削除
void StudioAnimationSpeed::DeletePoint(AnimationSpeedClipData& clip, int id)
{
	clip.points.erase(
		std::remove_if(clip.points.begin(), clip.points.end(),
			[id](const AnimationSpeedPoint& p) { return p.id == id && !p.isEndpoint; }),
		clip.points.end());

	if (selectedPointId == id)
	{
		selectedPointId = -1;
	}
}
// 削除ボタン
void StudioAnimationSpeed::DrawDeleteButton(const char* label, const std::function<void()>& onDelete)
{
	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

	bool pressed = ImGui::Button(label, ImVec2(-1.0f, 0.0f));

	ImGui::PopStyleColor(3);

	if (pressed)
	{
		onDelete();
	}
}

// 秒数を0.0～1.0に
float StudioAnimationSpeed::TimeToNormalized(float time, float clipLength)
{
	if (clipLength <= 0.0001f) return 0.0f;
	return std::clamp(time / clipLength, 0.0f, 1.0f);
}

// 0.0～1.0を秒数に
float StudioAnimationSpeed::NormalizedToTime(float normalized, float clipLength)
{
	return normalized * clipLength;
}

// 保存
void StudioAnimationSpeed::Save(const std::string& modelPath)
{
	if (modelPath.empty()) return;

	std::string savePath = GetAnimationSpeedFilePath(modelPath);

	// フォルダが無ければ作成する
	std::filesystem::create_directories("Data/Json/AnimSpeed");

	std::ofstream ofs(savePath);
	if (ofs.is_open())
	{
		ofs << animationSpeedData.ToJson().dump(4);
	}
}

// モデルパスから保存先のパスを組み立てる
std::string StudioAnimationSpeed::GetAnimationSpeedFilePath(const std::string& modelPath)
{
	std::filesystem::path p(modelPath);
	std::string modelName = p.stem().string();

	return "Data/Json/AnimSpeed/" + modelName + ".animspeed";
}

// アニメーション一覧を更新する
void StudioAnimationSpeed::RefreshAvailableAnimations(entt::registry& registry)
{
	availableAnimations.clear();

	auto view = registry.view<SkinnedMeshComponent>();
	if (view.begin() == view.end()) return;

	auto entity = *view.begin();
	auto& skinned = view.get<SkinnedMeshComponent>(entity);

	if (!skinned.model) return;

	for (const auto& anim : skinned.model->GetAnimations())
	{
		availableAnimations.push_back(anim.name);
	}
}