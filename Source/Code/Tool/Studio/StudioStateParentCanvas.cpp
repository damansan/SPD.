#include "StudioStateParentCanvas.h"
#include <imgui.h>

void StudioStateParentCanvas::Initialize()
{
	// ノードエディタのコンテキストを作成
	canvas.Initialize();
}

// キャンバスを描く
void StudioStateParentCanvas::Draw(StateMachineData& graph)
{
	data = &graph;

	// キャンバス作成
	canvas.Begin("StateParentCanvas");

	// 削除や読み込みの後はノードエディタ側に残った選択を消す
	if (clearSelectionRequested)
	{
		ed::ClearSelection();
		clearSelectionRequested = false;
	}

	// ノードの矩形は毎フレーム作り直す
	nodeRects.clear();

	// ノード描画
	DrawEntryNode();
	DrawStateNodes();

	// 位置の反映は、読み込み直後の1フレームだけ
	needsApplyPositions = false; 

	// リンク描画
	DrawLinks();
	DrawPendingLink();

	// メニュー描画
	DrawBackgroundMenu();
	DrawNodeMenu();

	// 入力更新
	UpdateLinkCreation();
	UpdateSelection();

	// キャンバス終了
	canvas.End();
}

// 選択している物を削除
void StudioStateParentCanvas::DeleteSelection(StateMachineData& graph)
{
	// 選択中のリンクと遷移先のエントリーを削除
	if (selectedLinkId != -1)
	{
		graph.RemoveTransition(selectedLinkId);
		selectedLinkId = -1;
		return;
	}

	// 選択中のステート削除 (Entryは削除不可)
	for (int key : selectedNodeKeys)
	{
		if (key != StateMachineData::kEntryId)
		{
			graph.RemoveState(key);
		}
	}
	selectedNodeKeys.clear();
	clearSelectionRequested = true;
}

// 読み込み・新規作成のときに、選択などを戻す
void StudioStateParentCanvas::Reset()
{
	needsApplyPositions = true;
	selectedNodeKeys.clear();
	selectedLinkId = -1;
	pendingFromKey = -1;
	contextNodeKey = -1;
	clearSelectionRequested = true;
}

// インスペクター描画
void StudioStateParentCanvas::DrawInspector(StateMachineData& data)
{
	// 描画中だけ、受け取ったデータを指す
	this->data = &data;

	// リンク選択
	if (selectedLinkId != -1)
	{
		DrawLinkInspector(selectedLinkId);
		return;
	}

	// ノード複数選択
	if (selectedNodeKeys.size() > 1)
	{
		DrawMultiStateInspector();
		return;
	}

	// ノード1つ選択
	if (selectedNodeKeys.size() == 1)
	{
		int key = selectedNodeKeys[0];
		if (key == StateMachineData::kEntryId)
		{
			DrawEntryInspector();
			return;
		}
		if (StateNode* state = data.FindState(key))
		{
			DrawStateInspector(*state);
			return;
		}
	}

	// 未選択
	ImGui::TextDisabled("Nothing selected.");
}

// エントリーノード描画
void StudioStateParentCanvas::DrawEntryNode()
{
	ed::NodeId nodeId = ToNodeId(StateMachineData::kEntryId);
	
	// 読み込み直後はデータの位置にノードを反映
	if (needsApplyPositions)
	{
		ed::SetNodePosition(nodeId, ImVec2(data->entry.editorPos.x, data->entry.editorPos.y));
	}

	// Entryは緑で描画
	ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(46, 125, 50, 255));
	ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(27, 94, 32, 255));
	ed::BeginNode(nodeId);
	ImGui::Text("Entry");
	ImGui::Dummy(ImVec2(80.0f, 0.0f));
	ed::EndNode();
	ed::PopStyleColor(2);

	// リンクの計算に使う矩形を記録
	nodeRects[StateMachineData::kEntryId] = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };

	// ノードを動かしたら、データの位置に書き戻す
	ImVec2 pos = ed::GetNodePosition(nodeId);
	data->entry.editorPos = { pos.x, pos.y };
}

// ステート描画
void StudioStateParentCanvas::DrawStateNodes()
{
	for (auto& s : data->states)
	{
		ed::NodeId nodeId = ToNodeId(s.id);

		// 読み込み直後はデータの位置にノードを反映
		if (needsApplyPositions)
		{
			ed::SetNodePosition(nodeId, ImVec2(s.editorPos.x, s.editorPos.y));
		}

		// State毎の色を書く
		const float* c = s.editorColor;
		ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(c[0], c[1], c[2], 1.0f));
		ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(c[0] * 0.6f, c[1] * 0.6f, c[2] * 0.6f, 1.0f));
		ed::BeginNode(nodeId);
		ImGui::Text("%s", s.name.c_str());
		ImGui::Dummy(ImVec2(80.0f, 0.0f));
		ed::EndNode();
		ed::PopStyleColor(2);

		// リンクの計算に使う矩形を記録
		nodeRects[s.id] = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };

		// ノードを動かしたら、データの位置に書き戻す
		ImVec2 pos = ed::GetNodePosition(nodeId);
		s.editorPos = { pos.x, pos.y };
	}
}

// リンク描画
void StudioStateParentCanvas::DrawLinks()
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// クリック判定用の線は毎フレーム作り直す
	linkSegments.clear();

	// 2個のノードの間に複数リンクがあるとき何本目か記録する
	std::unordered_map<int64_t, int> laneCounts;

	// リンク1本分を描画
	auto drawOne = [&](int fromKey, const StateTransition& t)
		{
			// 遷移元と遷移先の組み合わせを1つの値にまとめて何本目かを数える
			int64_t pairKey = (static_cast<int64_t>(fromKey) << 32) | static_cast<uint32_t>(t.toStateId);
			int lane = laneCounts[pairKey]++;

			ImVec2 start, end;
			if (!CalcLinkPoints(fromKey, t.toStateId, lane, start, end)) return;

			bool selected = (t.id == selectedLinkId);
			bool invalid = (!t.always && t.condition.IsEmpty());

			// 正常(白色) 条件なし(赤色) 選択中(黄色)
			ImU32 color = IM_COL32(210, 210, 210, 255);
			if (invalid)  color = IM_COL32(230, 90, 90, 255);   // 条件の設定漏れ
			if (selected) color = IM_COL32(255, 200, 80, 255);  // 選択中

			StateGraphDraw::DrawArrow(drawList, start, end, color, selected ? 3.0f : 2.0f);

			// クリック判定のために描いたリンクを記録
			linkSegments.push_back({ t.id, start, end });
		};

	// Entryから出るリンク
	for (const auto& t : data->entry.transitions) drawOne(StateMachineData::kEntryId, t);
	
	// 各Stateから出るリンク
	for (const auto& s : data->states)
	{
		for (const auto& t : s.transitions) drawOne(s.id, t);
	}
}

// 作成中のリンク描画
void StudioStateParentCanvas::DrawPendingLink()
{
	if (pendingFromKey == -1) return;

	auto it = nodeRects.find(pendingFromKey);
	if (it == nodeRects.end()) return;

	// 遷移元のノードの縁から、マウスの位置まで線を引く
	ImVec2 mouse = ImGui::GetMousePos();
	ImVec2 start = StateGraphDraw::GetEdgePoint(it->second.first, it->second.second, mouse);
	StateGraphDraw::DrawArrow(ImGui::GetWindowDrawList(), start, mouse, IM_COL32(255, 200, 80, 255), 2.0f);
}

// 背景右クリックのメニュー
void StudioStateParentCanvas::DrawBackgroundMenu()
{
	ed::Suspend();

	// ポップを開くリクエスト
	if (ed::ShowBackgroundContextMenu())
	{
		pendingCreatePos = ImGui::GetMousePos(); // マウス位置保存
		ImGui::OpenPopup("StateParentBackgroundMenu");
	}

	// ポップ内容
	if (ImGui::BeginPopup("StateParentBackgroundMenu"))
	{
		// 右クリックした位置にStateを作る
		if (ImGui::MenuItem("Create State"))
		{
			ImVec2 canvasPos = ed::ScreenToCanvas(pendingCreatePos);
			int newId = data->AddState("New State", { canvasPos.x, canvasPos.y });
			ed::SetNodePosition(ToNodeId(newId), canvasPos);
		}
		ImGui::EndPopup();
	}

	ed::Resume();
}

// ノード右クリックのメニュー
void StudioStateParentCanvas::DrawNodeMenu()
{
	ed::Suspend();

	ed::NodeId nodeId;
	if (ed::ShowNodeContextMenu(&nodeId))
	{
		// メニューを閉じるまでどのノードのメニューかを記録
		contextNodeKey = ToNodeKey(nodeId);

		// ポップを開くリクエスト
		ImGui::OpenPopup("StateParentNodeMenu");
	}

	// ポップ内容
	if (ImGui::BeginPopup("StateParentNodeMenu"))
	{
		// リンク作成開始
		if (ImGui::MenuItem("Make Link"))
		{
			pendingFromKey = contextNodeKey;
		}
		ImGui::EndPopup();
	}

	ed::Resume();
}

// リンク作成中の入力
void StudioStateParentCanvas::UpdateLinkCreation()
{
	if (pendingFromKey == -1) return;

	// Escで取り消し
	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		pendingFromKey = -1;
		return;
	}

	// 
	if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return;

	// クリックした位置にノードがあればそこを行き先にする
	ed::NodeId hovered = ed::GetHoveredNode();
	if (hovered)
	{
		int toKey = ToNodeKey(hovered);

		// Entryへのリンクと、自分自身へのリンクは作らない
		if (toKey != StateMachineData::kEntryId && toKey != pendingFromKey)
		{
			int newId = data->AddTransition(pendingFromKey, toKey);
			if (newId != -1)
			{
				// 作ったリンクを選択状態にする
				selectedLinkId = newId;
				selectedNodeKeys.clear();
				ed::ClearSelection();
			}
		}
	}

	// ノード以外をクリックした場合も、作成は終わる
	pendingFromKey = -1;
}

// 選択の更新
void StudioStateParentCanvas::UpdateSelection()
{
	// Stateのノードをダブルクリックしたら、子のウィンドウを開く
	if (ed::NodeId doubleClicked = ed::GetDoubleClickedNode())
	{
		int key = ToNodeKey(doubleClicked);
		if (key != StateMachineData::kEntryId) openChildRequest = key;
	}

	// ノードの選択
	int count = ed::GetSelectedObjectCount();
	std::vector<ed::NodeId> nodeIds(count > 0 ? count : 1);
	int nodeCount = (count > 0) ? ed::GetSelectedNodes(nodeIds.data(), count) : 0;

	std::vector<int> keys;
	for (int i = 0; i < nodeCount; ++i)
	{
		int key = ToNodeKey(nodeIds[i]);
		// 削除した直後などで、データに無いノードが残っていても選択として扱わない
		if (IsExistingNode(key)) keys.push_back(key);
	}

	if (!keys.empty())
	{
		selectedNodeKeys = std::move(keys);
		selectedLinkId = -1;
	}
	else if (selectedLinkId == -1)
	{
		// ノードもリンクも選ばれていない
		selectedNodeKeys.clear();
	}

	// リンクのクリックの判定
	if (pendingFromKey != -1) return;
	if (!ed::IsBackgroundClicked()) return;

	// ノードエディタの中では、マウスの位置はキャンバスの座標で返る(線の座標と同じ)
	ImVec2 mouse = ImGui::GetMousePos();
	const float hitDistance = 6.0f;

	// クリックした位置から一番近いリンクを探す(遠すぎる場合は選ばない)
	int hitId = -1;
	float best = hitDistance;
	for (const auto& seg : linkSegments)
	{
		float d = StateGraphDraw::DistanceToSegment(mouse, seg.start, seg.end);
		if (d < best)
		{
			best = d;
			hitId = seg.linkId;
		}
	}

	// 線のないところをクリックしたら選択解除
	selectedLinkId = hitId;
	if (hitId != -1)
	{
		// リンクを選んだら、ノードの選択は外す
		selectedNodeKeys.clear();
		ed::ClearSelection();
	}
}

// データに存在するノードか
bool StudioStateParentCanvas::IsExistingNode(int key) const
{
	if (key == StateMachineData::kEntryId) return true;
	return std::as_const(*data).FindState(key) != nullptr;
}

// ステートの情報
void StudioStateParentCanvas::DrawStateInspector(StateNode& state)
{
	ImGui::Text("State");


	// 選ぶStateが変わったら、入力欄を今の名前で作り直す
	if (nameEditingStateId != state.id)
	{
		nameEditingStateId = state.id;
		strncpy_s(nameBuffer, state.name.c_str(), sizeof(nameBuffer) - 1);
		nameError.clear();
	}

	// 名前
	if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		if (data->RenameState(state.id, nameBuffer))
		{
			nameError.clear();
		}
		else
		{
			nameError = (nameBuffer[0] == '\0') ? "Name is empty." : "Name already used.";
		}
	}

	// 入力欄から離れたら、確定していない文字は今の名前に戻す
	if (ImGui::IsItemDeactivated() && state.name != nameBuffer)
	{
		strncpy_s(nameBuffer, state.name.c_str(), sizeof(nameBuffer) - 1);
	}
	if (!nameError.empty())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", nameError.c_str());
	}

	// 色
	ImGui::ColorEdit3("Color", state.editorColor);
	ImGui::Spacing();

	// 出ていくリンクの一覧
	DrawLinkList(state.id, state.transitions);
	ImGui::Spacing();

	// 子グラフへ移動
	if (ImGui::Button("Open Child Graph", ImVec2(-1.0f, 0.0f)))
	{
		openChildRequest = state.id;
	}

	// 削除
	if (StateGraphDraw::DeleteButton("Delete State"))
	{
		DeleteSelection(*data);
	}
}

// エントリーの設定
void StudioStateParentCanvas::DrawEntryInspector()
{
	ImGui::Text("Entry");
	ImGui::Separator();

	// 出ていくリンクの一覧
	DrawLinkList(StateMachineData::kEntryId, data->entry.transitions);
}

// 複数ステートの選択時
void StudioStateParentCanvas::DrawMultiStateInspector()
{
	ImGui::Text("%d States selected", static_cast<int>(selectedNodeKeys.size()));
	ImGui::Separator();

	// まとめて削除
	if (StateGraphDraw::DeleteButton("Delete States"))
	{
		DeleteSelection(*data);
	}
}

// リンクの設定
void StudioStateParentCanvas::DrawLinkInspector(int linkId)
{
	StateTransition* link = data->FindTransition(linkId);
	int fromKey = -1, index = -1, count = 0;
	if (!link || !FindLinkOwner(linkId, fromKey, index, count))
	{
		ImGui::TextDisabled("Nothing selected.");
		return;
	}

	ImGui::Text("Link");


	// 遷移元 → 遷移先
	ImGui::Text("%s  ->  %s", GetNodeName(fromKey).c_str(), GetNodeName(link->toStateId).c_str());

	// 優先度
	ImGui::Text("Priority: %d / %d", index + 1, count);	ImGui::Spacing();

	// 常に成立
	ImGui::Checkbox("Always", &link->always);

	// ブレンド
	ImGui::Checkbox("Blend", &link->blend.enabled);
	ImGui::BeginDisabled(!link->blend.enabled);
	ImGui::DragFloat("Blend Duration", &link->blend.duration, 0.01f, 0.0f, 5.0f, "%.2f s");
	ImGui::EndDisabled();
	ImGui::Spacing();

	// 条件の状態
	if (link->always)
	{
		ImGui::TextDisabled("Condition: Always");
	}
	else if (link->condition.IsEmpty())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Condition: Empty");
	}
	else
	{
		ImGui::Text("Condition: Set");
	}

	// 条件のウィンドウで編集する
	if (ImGui::Button("Edit Condition", ImVec2(-1.0f, 0.0f)))
	{
		editConditionRequest = { ConditionTarget::Kind::StateLink, -1, linkId };
	}

	// 削除
	if (StateGraphDraw::DeleteButton("Delete Link"))
	{
		DeleteSelection(*data);
	}

	// 同じ階層のリンクから、設定を写すためのリスト
	ConditionTarget self{ ConditionTarget::Kind::StateLink, -1, linkId };
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Text("Copy From");

	// 受け口に落とされたら、写す依頼を出す
	ConditionTarget source = ConditionCopyList::DrawDropTarget(self);
	if (source.IsValid()) copyRequest = { source, self };

	// 写す元のリスト
	ConditionCopyList::Draw(ConditionCopyList::CollectParent(*data, self));
}

// 出ていくリンクの一覧
void StudioStateParentCanvas::DrawLinkList(int fromKey, std::vector<StateTransition>& links)
{
	ImGui::Text("Links (priority order)");

	if (links.empty())
	{
		ImGui::TextDisabled("No links.");
		return;
	}

	// 入れ替えは描画が終わってから
	int moveId = -1;
	int moveOffset = 0;

	for (int i = 0; i < static_cast<int>(links.size()); ++i)
	{
		const StateTransition& t = links[i];
		ImGui::PushID(t.id);

		// 上下で優先度を入れ替える
		if (ImGui::ArrowButton("##Up", ImGuiDir_Up)) { moveId = t.id; moveOffset = -1; }
		ImGui::SameLine();
		if (ImGui::ArrowButton("##Down", ImGuiDir_Down)) { moveId = t.id; moveOffset = +1; }
		ImGui::SameLine();

		// 条件の設定漏れは赤で表示
		bool invalid = (!t.always && t.condition.IsEmpty());
		if (invalid) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));

		// クリックでそのリンクを選ぶ
		std::string label = std::to_string(i + 1) + ". -> " + GetNodeName(t.toStateId);
		if (ImGui::Selectable(label.c_str(), t.id == selectedLinkId))
		{
			selectedLinkId = t.id;
			selectedNodeKeys.clear();
			clearSelectionRequested = true; // キャンバス側のノードの選択も外す
		}

		if (invalid) ImGui::PopStyleColor();
		ImGui::PopID();
	}

	if (moveId != -1)
	{
		data->MoveTransition(moveId, moveOffset);
	}
	(void)fromKey;
}

// リンクの遷移元と、遷移元の中での何番目かを探す
bool StudioStateParentCanvas::FindLinkOwner(int linkId, int& outFromKey, int& outIndex, int& outCount) const
{
	auto search = [&](int fromKey, const std::vector<StateTransition>& list)
		{
			for (int i = 0; i < static_cast<int>(list.size()); ++i)
			{
				if (list[i].id == linkId)
				{
					outFromKey = fromKey;
					outIndex = i;
					outCount = static_cast<int>(list.size());
					return true;
				}
			}
			return false;
		};

	if (search(StateMachineData::kEntryId, data->entry.transitions)) return true;
	for (const auto& s : data->states)
	{
		if (search(s.id, s.transitions)) return true;
	}
	return false;
}

// ノードのキーから、表示用の名前を作る
std::string StudioStateParentCanvas::GetNodeName(int key) const
{
	if (key == StateMachineData::kEntryId) return "Entry";
	if (const StateNode* s = std::as_const(*data).FindState(key)) return s->name;
	return "(missing)";
}

// fromKeyからtoStateIdへのリンクがあるか
bool StudioStateParentCanvas::HasLink(int fromKey, int toStateId) const
{
	// 遷移元のリンクの一覧
	const std::vector<StateTransition>* list = nullptr;
	if (fromKey == StateMachineData::kEntryId)
	{
		list = &data->entry.transitions;
	}
	else if (const StateNode* s = std::as_const(*data).FindState(fromKey))
	{
		list = &s->transitions;
	}
	if (!list) return false;

	for (const auto& t : *list)
	{
		if (t.toStateId == toStateId) return true;
	}
	return false;
}

// 線の始点と終点を求める
bool StudioStateParentCanvas::CalcLinkPoints(int fromKey, int toStateId, int lane, ImVec2& outStart, ImVec2& outEnd) const
{
	auto fromIt = nodeRects.find(fromKey);
	auto toIt = nodeRects.find(toStateId);
	if (fromIt == nodeRects.end() || toIt == nodeRects.end()) return false;

	StateGraphDraw::CalcLinkPoints(
		fromIt->second.first, fromIt->second.second, fromKey,
		toIt->second.first, toIt->second.second, toStateId,
		HasLink(toStateId, fromKey), lane,
		outStart, outEnd);
	return true;
}

// ノードのキー → ノードエディタのID
ed::NodeId StudioStateParentCanvas::ToNodeId(int key)
{
	if (key == StateMachineData::kEntryId) return ed::NodeId(static_cast<uint64_t>(kEntryNodeId));
	return ed::NodeId(static_cast<uint64_t>(key));
}

// ノードエディタのID → ノードのキー
int StudioStateParentCanvas::ToNodeKey(ed::NodeId nodeId)
{
	if (nodeId.Get() == static_cast<uint64_t>(kEntryNodeId)) return StateMachineData::kEntryId;
	return static_cast<int>(nodeId.Get());
}