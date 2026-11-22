#include "StudioStateGraph.h"
#include <algorithm>
#include <imgui.h>
#include <json.hpp>
#include <cmath>
#include <cfloat>
#include <memory>

#include "Engine/ECS/Components/SkinnedMeshComponent.h"
#include "Engine/ECS/Components/AnimatorComponent.h"

namespace
{
	// 次に作るウィンドウの持ち主を、Studioのビューポートにする
	void SetNextWindowOwner(ImGuiID ownerViewportId)
	{
		if (ownerViewportId == 0) return;

		ImGuiWindowClass windowClass;
		windowClass.ParentViewportId = ownerViewportId;
		ImGui::SetNextWindowClass(&windowClass);
	}

	// Return以外のノードがあるか(写すと消えてしまう物があるか)
	bool HasNodesBesidesReturn(const ConditionGraph& condition)
	{
		return !condition.variableNodes.empty()
			|| !condition.getterNodes.empty()
			|| !condition.operatorNodes.empty()
			|| !condition.logicNodes.empty()
			|| !condition.notNodes.empty();
	}
}

void StudioStateGraph::Initialize()
{
	// 親階層キャンバス初期化
	parentCanvas.Initialize();
	// 条件のキャンバス初期化
	conditionCanvas.Initialize();
}

void StudioStateGraph::DrawGUI(entt::registry& registry, StudioContext& context)
{
	// 子のウィンドウに渡すアニメーションの一覧
	RefreshAnimationNames(registry);

	std::string panelName = "StateGraph##" + std::to_string(windowId);
	bool open = ImGui::Begin(panelName.c_str());

	// StaticModelは弾く
	if (open && !HasAnimatedModel(registry))
	{
		ImGui::Text("No skinned model.");
	}
	else if (open)
	{
		DrawToolbar(context);

		ImGui::Columns(2, "StateGraphColumns", true);
		if (!columnWidthInitialized)
		{
			ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.75f);
			columnWidthInitialized = true;
		}

		// 左：グラフ
		parentCanvas.Draw(data);

		ImGui::NextColumn();

		// 右：インスペクター
		parentCanvas.DrawInspector(data);

		// 選択している物の削除(文字を入力中は、Deleteキーを文字の削除に使うので除く)
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
			!ImGui::GetIO().WantTextInput &&
			ImGui::IsKeyPressed(ImGuiKey_Delete))
		{
			parentCanvas.DeleteSelection(data);
		}

		ImGui::Columns(1);

		// 親階層から子階層のキャンバスを開く依頼があれば開く
		int openRequest = parentCanvas.ConsumeOpenChildRequest();
		if (openRequest != -1) OpenChildWindow(openRequest);

		// 親のリンクの条件を編集する依頼があれば開く
		ConditionTarget conditionRequest = parentCanvas.ConsumeEditConditionRequest();
		if (conditionRequest.IsValid()) OpenConditionWindow(conditionRequest);

		// 親のリンクの設定を写す依頼
		ConditionCopyRequest copyRequest = parentCanvas.ConsumeCopyRequest();
		if (copyRequest.IsValid()) RequestCopy(copyRequest);
	}

	ImGui::End();

	// 子階層ウィンドウ
	DrawChildWindows(context.studioViewportId);
	// 条件のウィンドウ
	DrawConditionWindow(context.studioViewportId);

	// 上書きの確認
	DrawCopyConfirmPopup();
}

// モデルが変わったときに呼ぶ
void StudioStateGraph::OnModelChanged(StudioContext& context)
{
	// 選択などを戻す
	parentCanvas.Reset();
	childCanvases.clear();
	CloseConditionWindow();
	pendingCopy = {};
	openCopyConfirm = false;

	// このモデルのグラフを読む
	Load(context);
}

// ツールバー
void StudioStateGraph::DrawToolbar(StudioContext& context)
{
	if (ImGui::Button("Save"))
	{
		Save(context);
	}
	ImGui::SameLine();
	const std::string path = StateMachineData::GetFilePath(context.currentModelPath);
	if (fileCache.GetStatus(path) == JsonFileStatus::Missing)
	{
		ImGui::TextDisabled("Not saved yet.");
	}
	else
	{
		ImGui::TextDisabled("%s", path.c_str());
	}

	// ファイルの読み書きの失敗など
	if (!message.empty())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", message.c_str());
	}
	// 検証エラー
	if (hasErrors)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Validation errors. See Log.");
	}

	ImGui::Separator();
}


// 保存
void StudioStateGraph::Save(StudioContext& context)
{
	if (context.currentModelPath.empty()) return;
	const std::string path = StateMachineData::GetFilePath(context.currentModelPath);

	// 作りかけでも保存はできる、エラーの有無はツールバーに表示する
	hasErrors = !data.Validate(path);

	GraphFile file;
	file.data = data;
	file.valid = !hasErrors;

	const bool saved = fileCache.Save(path, file,
		[](const GraphFile& f) { return f.data.ToJson(); });

	message = saved ? "" : "Failed to save file.";
}
// 読み込み
void StudioStateGraph::Load(StudioContext& context)
{
	if (context.currentModelPath.empty()) return;
	const std::string path = StateMachineData::GetFilePath(context.currentModelPath);

	// 無いのは「まだ作っていない」だけなので、ログは出さない
	const GraphFile& file = fileCache.GetOrLoad(path,
		[&path](GraphFile& f, const nlohmann::json& j) { f.valid = f.data.FromJson(j, path); },
		JsonMissingLog::Silent);
	const JsonFileStatus status = fileCache.GetStatus(path);

	// 検証でエラーがあっても、直すために読み込んだ状態で開く
	data = file.data;
	hasErrors = (status == JsonFileStatus::Loaded) && !file.valid;
	message = (status == JsonFileStatus::Broken) ? "Invalid json." : "";
}

// モデルが持っているアニメーションの名前の一覧を作り直す
void StudioStateGraph::RefreshAnimationNames(entt::registry& registry)
{
	animationNames.clear();

	auto view = registry.view<SkinnedMeshComponent>();
	if (view.begin() == view.end()) return;

	auto& skinned = view.get<SkinnedMeshComponent>(*view.begin());
	if (!skinned.model) return;

	for (const auto& anim : skinned.model->GetAnimations())
	{
		animationNames.push_back(anim.name);
	}
}

// アニメーションを再生できるモデルか
bool StudioStateGraph::HasAnimatedModel(entt::registry& registry)
{
	auto view = registry.view<SkinnedMeshComponent, AnimatorComponent>();
	return view.begin() != view.end();
}

// 子のウィンドウを開く
void StudioStateGraph::OpenChildWindow(int stateId)
{
	// すでに開いていれば、新しく開かずに手前に出す
	for (const auto& child : childCanvases)
	{
		if (child->GetStateId() == stateId)
		{
			focusChildStateId = stateId;
			return;
		}
	}

	auto child = std::make_unique<StudioStateChildCanvas>(stateId);
	child->Initialize();
	childCanvases.push_back(std::move(child));
	focusChildStateId = stateId;
}

// 開いている子のウィンドウを描く
void StudioStateGraph::DrawChildWindows(ImGuiID ownerViewportId)
{
	for (auto it = childCanvases.begin(); it != childCanvases.end(); )
	{
		StudioStateChildCanvas& child = **it;

		// Stateが消えていれば閉じる
		const StateNode* state = std::as_const(data).FindState(child.GetStateId());
		if (!state)
		{
			it = childCanvases.erase(it);
			continue;
		}

		// 表示はState名、IDはStateのIDで作る
		std::string title = "Child: " + state->name +
			"###StateChild_" + std::to_string(windowId) + "_" + std::to_string(state->id);

		// タイトルバーをStateの色にする
		const float* c = state->editorColor;
		ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(c[0] * 0.7f, c[1] * 0.7f, c[2] * 0.7f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(c[0], c[1], c[2], 1.0f));

		// 開いた直後だけ大きさを決める
		ImGui::SetNextWindowSize(ImVec2(700.0f, 450.0f), ImGuiCond_FirstUseEver);
		if (focusChildStateId == state->id)
		{
			ImGui::SetNextWindowFocus();
			focusChildStateId = -1;
		}

		// アプリの外に出したとき、Studioより手前に出るようにする
		SetNextWindowOwner(ownerViewportId);

		bool open = true;
		bool visible = ImGui::Begin(title.c_str(), &open);
		ImGui::PopStyleColor(2);

		if (visible)
		{
			child.Draw(data, animationNames);
		}
		ImGui::End();

		// 子のリンクの条件を編集する依頼があれば開く
		ConditionTarget conditionRequest = child.ConsumeEditConditionRequest();
		if (conditionRequest.IsValid()) OpenConditionWindow(conditionRequest);

		// 子のリンクの設定を写す依頼
		ConditionCopyRequest copyRequest = child.ConsumeCopyRequest();
		if (copyRequest.IsValid()) RequestCopy(copyRequest);

		// ×で閉じられたら、一覧から外す
		if (!open) it = childCanvases.erase(it);
		else ++it;
	}
}

// 条件のウィンドウを開く
void StudioStateGraph::OpenConditionWindow(const ConditionTarget& target)
{
	// 別のリンクに切り替えるときは、実体が同じ場所でも必ず画面をリセットする
	if (target != conditionTarget) conditionPointer = nullptr;

	conditionTarget = target;
	focusConditionWindow = true;
}

// 条件のウィンドウを描く
void StudioStateGraph::DrawConditionWindow(ImGuiID ownerViewportId)
{
	if (!conditionTarget.IsValid()) return;

	// 編集する条件の実体を、毎フレーム探し直す
	std::string description;
	ImVec4 color;
	ConditionGraph* condition = ResolveCondition(conditionTarget, description, color);

	// リンクが消えていたら閉じる
	if (!condition)
	{
		CloseConditionWindow();
		return;
	}

	// 実体の場所が変わったら条件の画面をリセットする
	if (condition != conditionPointer)
	{
		conditionCanvas.SetTarget(condition);
		conditionPointer = condition;
	}

	std::string title = "Condition: " + description + "###StateCondition_" + std::to_string(windowId);

	// タイトルバーを、リンクの持ち主のStateの色にする
	ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(color.x * 0.7f, color.y * 0.7f, color.z * 0.7f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_TitleBgActive, color);

	// 開いた直後だけ大きさを決める
	ImGui::SetNextWindowSize(ImVec2(800.0f, 500.0f), ImGuiCond_FirstUseEver);
	if (focusConditionWindow)
	{
		ImGui::SetNextWindowFocus();
		focusConditionWindow = false;
	}

	// アプリの外に出したとき、Studioより手前に出るようにする
	SetNextWindowOwner(ownerViewportId);

	bool open = true;
	bool visible = ImGui::Begin(title.c_str(), &open);
	ImGui::PopStyleColor(2);

	if (visible)
	{
		// 左：条件のキャンバス、右：条件のインスペクター
		ImGui::Columns(2, "StateConditionColumns", true);
		if (!conditionColumnInitialized)
		{
			ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.7f);
			conditionColumnInitialized = true;
		}

		conditionCanvas.Draw();
		ImGui::NextColumn();
		conditionCanvas.DrawInspector();

		ImGui::Columns(1);
	}
	ImGui::End();

	// ×で閉じられた
	if (!open) CloseConditionWindow();
}

// 条件のウィンドウを閉じる
void StudioStateGraph::CloseConditionWindow()
{
	conditionTarget = {};
	conditionPointer = nullptr;
	conditionCanvas.SetTarget(nullptr);
}

// 編集する条件の実体を探す
ConditionGraph* StudioStateGraph::ResolveCondition(const ConditionTarget& target, std::string& outTitle, ImVec4& outColor)
{
	switch (target.kind)
	{
	case ConditionTarget::Kind::StateLink:
	{
		// 親のリンク：遷移元 → 遷移先
		StateTransition* t = data.FindTransition(target.linkId);
		if (!t) return nullptr;

		std::string fromName;
		if (!FindTransitionSource(t->id, fromName, outColor)) return nullptr;

		const StateNode* to = std::as_const(data).FindState(t->toStateId);
		outTitle = fromName + " -> " + (to ? to->name : "(missing)");
		return &t->condition;
	}

	case ConditionTarget::Kind::EntryLink:
	case ConditionTarget::Kind::ClipLink:
	{
		// 子のリンク：State名 / 元 → 先
		StateNode* state = data.FindState(target.stateId);
		if (!state) return nullptr;

		const float* c = state->editorColor;
		outColor = ImVec4(c[0], c[1], c[2], 1.0f);

		auto clipName = [state](int clipId) -> std::string
			{
				const ClipNode* clip = state->child.FindClip(clipId);
				return clip ? clip->name : "(missing)";
			};

		if (target.kind == ConditionTarget::Kind::EntryLink)
		{
			for (auto& l : state->child.entryLinks)
			{
				if (l.id != target.linkId) continue;

				std::string fromName;
				ImVec4 unused;
				FindTransitionSource(l.entryTransitionId, fromName, unused);
				outTitle = state->name + " / Entry(" + fromName + ") -> " + clipName(l.toClipId);
				return &l.condition;
			}
		}
		else
		{
			for (auto& l : state->child.clipLinks)
			{
				if (l.id != target.linkId) continue;

				outTitle = state->name + " / " + clipName(l.fromClipId) + " -> " + clipName(l.toClipId);
				return &l.condition;
			}
		}
		return nullptr;
	}

	default:
		return nullptr;
	}
}

// 親のリンクのIDから、遷移元の名前と色を探す
bool StudioStateGraph::FindTransitionSource(int transitionId, std::string& outName, ImVec4& outColor) const
{
	// Entryから出るリンク
	for (const auto& t : data.entry.transitions)
	{
		if (t.id != transitionId) continue;
		outName = "Entry";
		outColor = ImVec4(46.0f / 255.0f, 125.0f / 255.0f, 50.0f / 255.0f, 1.0f);
		return true;
	}

	// 各Stateから出るリンク
	for (const auto& s : data.states)
	{
		for (const auto& t : s.transitions)
		{
			if (t.id != transitionId) continue;
			outName = s.name;
			outColor = ImVec4(s.editorColor[0], s.editorColor[1], s.editorColor[2], 1.0f);
			return true;
		}
	}
	return false;
}

// リンクの設定の場所を探す
bool StudioStateGraph::ResolveLinkSettings(const ConditionTarget& target, LinkSettingsRef& out)
{
	out = {};

	switch (target.kind)
	{
	case ConditionTarget::Kind::StateLink:
	{
		// 親のリンク：条件、always、ブレンド
		StateTransition* t = data.FindTransition(target.linkId);
		if (!t) return false;
		out = { &t->condition, &t->always, &t->blend };
		return true;
	}

	case ConditionTarget::Kind::EntryLink:
	{
		// 子のエントリーのリンク：条件、always(ブレンドは親のリンクが持つので無い)
		StateNode* state = data.FindState(target.stateId);
		if (!state) return false;
		for (auto& l : state->child.entryLinks)
		{
			if (l.id != target.linkId) continue;
			out = { &l.condition, &l.always, nullptr };
			return true;
		}
		return false;
	}

	case ConditionTarget::Kind::ClipLink:
	{
		// 子のクリップのリンク：条件、always、ブレンド
		StateNode* state = data.FindState(target.stateId);
		if (!state) return false;
		for (auto& l : state->child.clipLinks)
		{
			if (l.id != target.linkId) continue;
			out = { &l.condition, &l.always, &l.blend };
			return true;
		}
		return false;
	}

	default:
		return false;
	}
}

// 写す依頼を受け取る
void StudioStateGraph::RequestCopy(const ConditionCopyRequest& request)
{
	LinkSettingsRef dst;
	if (!ResolveLinkSettings(request.target, dst)) return;

	// 先にReturn以外のノードがあれば、確認してから写す
	if (HasNodesBesidesReturn(*dst.condition))
	{
		pendingCopy = request;
		openCopyConfirm = true;
		return;
	}

	CopyLinkSettings(request);
}

// 写す
void StudioStateGraph::CopyLinkSettings(const ConditionCopyRequest& request)
{
	if (request.source == request.target) return;

	// 確認の間にリンクが消えている場合もあるので、ここで探し直す
	LinkSettingsRef src, dst;
	if (!ResolveLinkSettings(request.source, src)) return;
	if (!ResolveLinkSettings(request.target, dst)) return;

	// 条件とalwaysを写す
	*dst.condition = *src.condition;
	*dst.always = *src.always;

	// ブレンドは、両方が持っているときだけ写す
	if (src.blend && dst.blend) *dst.blend = *src.blend;

	// 写した先を条件ウィンドウで開いていれば、画面を付け直す
	if (conditionTarget == request.target) conditionPointer = nullptr;
}

// 上書きの確認のポップアップ
void StudioStateGraph::DrawCopyConfirmPopup()
{
	// Studioのウィンドウごとに名前を分ける
	std::string popupName = "Copy Link Settings##" + std::to_string(windowId);

	// 依頼が来ていれば開く
	if (openCopyConfirm)
	{
		ImGui::OpenPopup(popupName.c_str());
		openCopyConfirm = false;
	}

	// 画面の中央に出す
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (!ImGui::BeginPopupModal(popupName.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;

	// 元と先の名前
	std::string fromTitle, toTitle;
	ImVec4 unused;
	bool exists = ResolveCondition(pendingCopy.source, fromTitle, unused) != nullptr
		&& ResolveCondition(pendingCopy.target, toTitle, unused) != nullptr;
	if (!exists)
	{
		pendingCopy = {};
		ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
		return;
	}

	ImGui::Text("The condition of this link will be replaced:");
	ImGui::Text("    %s", toTitle.c_str());
	ImGui::Text("Copy from:");
	ImGui::Text("    %s", fromTitle.c_str());
	ImGui::Spacing();

	if (ImGui::Button("Replace", ImVec2(120.0f, 0.0f)))
	{
		CopyLinkSettings(pendingCopy);
		pendingCopy = {};
		ImGui::CloseCurrentPopup();
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
	{
		pendingCopy = {};
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
}