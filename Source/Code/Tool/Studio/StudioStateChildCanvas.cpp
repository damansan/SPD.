#include "StudioStateChildCanvas.h"
#include <utility>
#include <algorithm>
#include <cctype>
#include <imgui.h>

void StudioStateChildCanvas::Initialize()
{
	// ノードエディタのコンテキストを作る
	canvas.Initialize();
}

// キャンバスを描く
void StudioStateChildCanvas::Draw(StateMachineData& data, const std::vector<std::string>& animations)
{
	// 描画中だけ、受け取ったデータを指す
	this->data = &data;
	state = data.FindState(stateId);
	if (!state) return;

	CollectEntries();

	ImGui::Columns(2, "StateChildColumns", true);
	if (!columnWidthInitialized)
	{
		ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.7f);
		columnWidthInitialized = true;
	}

	// キャンバス作成
	canvas.Begin("StateChildCanvas");

	// 削除や読み込みの後はノードエディタ側に残った選択を消す
	if (clearSelectionRequested)
	{
		ed::ClearSelection();
		clearSelectionRequested = false;
	}

	// ノードの矩形は毎フレーム作り直す
	nodeRects.clear();

	// ノード描画
	DrawEntryNodes();
	DrawClipNodes();

	// 位置の反映は、開いた直後の1フレームだけ
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

	ImGui::NextColumn();

	// 右：インスペクター
	DrawInspector(animations);

	// 選択している物の削除(文字を入力中は除く)
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
		!ImGui::GetIO().WantTextInput &&
		ImGui::IsKeyPressed(ImGuiKey_Delete))
	{
		DeleteSelection(data);
	}

	ImGui::Columns(1);
}

// 選択している物を削除
void StudioStateChildCanvas::DeleteSelection(StateMachineData& data)
{
	// 選択中のリンクを削除
	if (selectedLinkId != -1)
	{
		const auto& entryLinks = state->child.entryLinks;
		bool isEntryLink = std::any_of(entryLinks.begin(), entryLinks.end(),
			[this](const EntryLink& l) { return l.id == selectedLinkId; });

		if (isEntryLink) data.RemoveEntryLink(stateId, selectedLinkId);
		else             data.RemoveClipLink(stateId, selectedLinkId);

		selectedLinkId = -1;
		return;
	}

	// 選択中のクリップを削除 (Entryは削除不可)
	for (int key : selectedNodeKeys)
	{
		if (!IsEntry(key))
		{
			data.RemoveClip(stateId, key);
		}
	}
	selectedNodeKeys.clear();
	clearSelectionRequested = true;
}

// 入ってくるリンクからエントリーの一覧を作る
void StudioStateChildCanvas::CollectEntries()
{
	entries.clear();

	// 親のEntryから入ってくるリンク
	for (const auto& t : data->entry.transitions)
	{
		if (t.toStateId == stateId) entries.push_back({ t.id, StateMachineData::kEntryId });
	}

	// 各Stateから入ってくるリンク
	for (const auto& s : data->states)
	{
		for (const auto& t : s.transitions)
		{
			if (t.toStateId == stateId) entries.push_back({ t.id, s.id });
		}
	}
}

// エントリーノード描画
void StudioStateChildCanvas::DrawEntryNodes()
{
	for (int i = 0; i < static_cast<int>(entries.size()); ++i)
	{
		const EntryInfo& e = entries[i];
		ed::NodeId nodeId(static_cast<uint64_t>(e.transitionId));

		// 位置がまだ無ければ、左側に縦に並べて置く
		auto& posMap = state->child.entryEditorPos;
		if (posMap.find(e.transitionId) == posMap.end())
		{
			posMap[e.transitionId] = { 50.0f, 100.0f + 80.0f * i };
			ed::SetNodePosition(nodeId, ImVec2(posMap[e.transitionId].x, posMap[e.transitionId].y));
		}
		else if (needsApplyPositions)
		{
			ed::SetNodePosition(nodeId, ImVec2(posMap[e.transitionId].x, posMap[e.transitionId].y));
		}

		// どのクリップにもつながっていないエントリーは、枠を赤くする
		bool connected = std::any_of(state->child.entryLinks.begin(), state->child.entryLinks.end(),
			[&e](const EntryLink& l) { return l.entryTransitionId == e.transitionId; });

		// エントリーは緑で描く
		ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(46, 125, 50, 255));
		ed::PushStyleColor(ed::StyleColor_NodeBorder, connected ? ImColor(27, 94, 32, 255) : ImColor(230, 90, 90, 255));
		ed::BeginNode(nodeId);
		ImGui::Text("From: %s", GetSourceName(e.fromKey).c_str());
		ImGui::Dummy(ImVec2(80.0f, 0.0f)); // ノードの最小の幅
		ed::EndNode();
		ed::PopStyleColor(2);

		// リンクの計算に使う矩形を覚えておく
		nodeRects[e.transitionId] = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };

		// ノードを動かしたら、データの位置に書き戻す
		ImVec2 pos = ed::GetNodePosition(nodeId);
		posMap[e.transitionId] = { pos.x, pos.y };
	}
}

// クリップノード描画
void StudioStateChildCanvas::DrawClipNodes()
{
	for (auto& c : state->child.clips)
	{
		ed::NodeId nodeId(static_cast<uint64_t>(c.id));

		// 読み込み直後はデータの位置にノードを反映
		if (needsApplyPositions)
		{
			ed::SetNodePosition(nodeId, ImVec2(c.editorPos.x, c.editorPos.y));
		}

		ed::BeginNode(nodeId);
		ImGui::Text("%s", c.name.c_str());
		// 再生するアニメーション(未設定なら灰色で表示)
		ImGui::TextDisabled("%s", c.clipName.empty() ? "(no clip)" : c.clipName.c_str());
		ImGui::Dummy(ImVec2(80.0f, 0.0f)); // ノードの最小の幅
		ed::EndNode();

		// リンクの計算に使う矩形を記録
		nodeRects[c.id] = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };

		// ノードを動かしたら、データの位置に書き戻す
		ImVec2 pos = ed::GetNodePosition(nodeId);
		c.editorPos = { pos.x, pos.y };
	}
}

// リンク描画
void StudioStateChildCanvas::DrawLinks()
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// クリック判定用の線は毎フレーム作り直す
	linkSegments.clear();

	// 2個のノードの間に複数リンクがあるとき何本目か記録する
	std::unordered_map<int64_t, int> laneCounts;

	// リンク1本分を描く
	auto drawOne = [&](int linkId, int fromKey, int toKey, bool hasReverse, bool always, const ConditionGraph& condition)
		{
			// 遷移元と遷移先の組み合わせを1つの値にまとめて何本目かを数える
			int64_t pairKey = (static_cast<int64_t>(fromKey) << 32) | static_cast<uint32_t>(toKey);
			int lane = laneCounts[pairKey]++;

			ImVec2 start, end;
			if (!CalcLinkPoints(fromKey, toKey, hasReverse, lane, start, end)) return;

			bool selected = (linkId == selectedLinkId);
			bool invalid = (!always && condition.IsEmpty());

			// 正常(白色) 条件なし(赤色) 選択中(黄色)
			ImU32 color = IM_COL32(210, 210, 210, 255);
			if (invalid)  color = IM_COL32(230, 90, 90, 255);
			if (selected) color = IM_COL32(255, 200, 80, 255);

			StateGraphDraw::DrawArrow(drawList, start, end, color, selected ? 3.0f : 2.0f);

			// クリック判定のために描いたリンクを記録
			linkSegments.push_back({ linkId, start, end });
		};

	// Entryから出るリンク
	for (const auto& l : state->child.entryLinks)
	{
		drawOne(l.id, l.entryTransitionId, l.toClipId, false, l.always, l.condition);
	}

	// 各Stateから出るリンク
	for (const auto& l : state->child.clipLinks)
	{
		drawOne(l.id, l.fromClipId, l.toClipId, HasClipLink(l.toClipId, l.fromClipId), l.always, l.condition);
	}
}

// 作成中のリンク描画
void StudioStateChildCanvas::DrawPendingLink()
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
void StudioStateChildCanvas::DrawBackgroundMenu()
{
	ed::Suspend();

	// ポップを開くリクエスト
	if (ed::ShowBackgroundContextMenu())
	{
		pendingCreatePos = ImGui::GetMousePos(); // マウス位置保存
		ImGui::OpenPopup("StateChildBackgroundMenu");
	}

	// ポップ内容
	if (ImGui::BeginPopup("StateChildBackgroundMenu"))
	{
		// 右クリックした位置にクリップを作る
		if (ImGui::MenuItem("Clip"))
		{
			ImVec2 canvasPos = ed::ScreenToCanvas(pendingCreatePos);
			int newId = data->AddClip(stateId, "New Clip", { canvasPos.x, canvasPos.y });
			if (newId != -1)
			{
				ed::SetNodePosition(ed::NodeId(static_cast<uint64_t>(newId)), canvasPos);
			}
		}
		ImGui::EndPopup();
	}

	ed::Resume();
}

// ノード右クリックのメニュー
void StudioStateChildCanvas::DrawNodeMenu()
{
	ed::Suspend();

	ed::NodeId nodeId;
	if (ed::ShowNodeContextMenu(&nodeId))
	{
		// メニューを閉じるまでどのクリップのメニューかを記録
		contextNodeKey = static_cast<int>(nodeId.Get());

		// ポップを開くリクエスト
		ImGui::OpenPopup("StateChildNodeMenu");
	}

	if (ImGui::BeginPopup("StateChildNodeMenu"))
	{
		// リンク作成開始
		if (ImGui::MenuItem("Link"))
		{
			pendingFromKey = contextNodeKey;
		}
		ImGui::EndPopup();
	}

	ed::Resume();
}

// リンク作成中の入力
void StudioStateChildCanvas::UpdateLinkCreation()
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
		int toKey = static_cast<int>(hovered.Get());

		// Entryへのリンクと、自分自身へのリンクは作らない
		if (!IsEntry(toKey) && toKey != pendingFromKey)
		{
			int newId = IsEntry(pendingFromKey)
				? data->AddEntryLink(stateId, pendingFromKey, toKey)  // エントリー から クリップ
				: data->AddClipLink(stateId, pendingFromKey, toKey);  // クリップ から クリップ

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
void StudioStateChildCanvas::UpdateSelection()
{
	// ノードの選択
	int count = ed::GetSelectedObjectCount();
	std::vector<ed::NodeId> nodeIds(count > 0 ? count : 1);
	int nodeCount = (count > 0) ? ed::GetSelectedNodes(nodeIds.data(), count) : 0;

	std::vector<int> keys;
	for (int i = 0; i < nodeCount; ++i)
	{
		int key = static_cast<int>(nodeIds[i].Get());
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

	// クリックした位置から一番近いリンクを探す(遠すぎる場合は選ばない)
	const float hitDistance = 6.0f;
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

// データに存在するノードか判定
bool StudioStateChildCanvas::IsExistingNode(int key) const
{
	return IsEntry(key) || state->child.FindClip(key) != nullptr;
}
// エントリーか判定
bool StudioStateChildCanvas::IsEntry(int key) const
{
	return std::any_of(entries.begin(), entries.end(),
		[key](const EntryInfo& e) { return e.transitionId == key; });
}

// インスペクター描画
void StudioStateChildCanvas::DrawInspector(const std::vector<std::string>& animations)
{
	// リンク選択中
	if (selectedLinkId != -1)
	{
		DrawLinkInspector(selectedLinkId);
		return;
	}

	// ノード複数選択
	if (selectedNodeKeys.size() > 1)
	{
		DrawMultiInspector();
		return;
	}

	// ノード1つ選択
	if (selectedNodeKeys.size() == 1)
	{
		int key = selectedNodeKeys[0];
		if (IsEntry(key))
		{
			DrawEntryInspector(key);
			return;
		}
		for (auto& c : state->child.clips)
		{
			if (c.id == key)
			{
				DrawClipInspector(c, animations);
				return;
			}
		}
	}

	// 未選択
	ImGui::TextDisabled("Nothing selected.");
}

// エントリーの設定
void StudioStateChildCanvas::DrawEntryInspector(int transitionId)
{
	ImGui::Text("Entry");
	ImGui::Separator();

	// どこから入ってくるか
	int fromKey = -1;
	for (const auto& e : entries)
	{
		if (e.transitionId == transitionId) fromKey = e.fromKey;
	}
	ImGui::Text("From: %s", GetSourceName(fromKey).c_str());

	// ブレンド表示
	if (const StateTransition* t = std::as_const(*data).FindTransition(transitionId))
	{
		if (t->blend.enabled) ImGui::TextDisabled("Blend: %.2f s (parent link)", t->blend.duration);
		else                  ImGui::TextDisabled("Blend: Off (parent link)");
	}
	ImGui::Separator();

	// 出ていくリンクの一覧
	DrawOutLinkList(transitionId, true);
}

// クリップの設定
void StudioStateChildCanvas::DrawClipInspector(ClipNode& clip, const std::vector<std::string>& animations)
{
	ImGui::Text("Clip");
	ImGui::Separator();

	// 選ぶクリップが変わったら、入力欄を今の名前で作り直す
	if (nameEditingClipId != clip.id)
	{
		nameEditingClipId = clip.id;
		strncpy_s(nameBuffer, clip.name.c_str(), sizeof(nameBuffer) - 1);
	}

	// 名前欄
	if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		if (nameBuffer[0] != '\0') clip.name = nameBuffer;
	}
	// 入力欄から離れたら、確定していない文字は今の名前に戻す
	if (ImGui::IsItemDeactivated() && clip.name != nameBuffer)
	{
		strncpy_s(nameBuffer, clip.name.c_str(), sizeof(nameBuffer) - 1);
	}
	ImGui::Separator();

	// 再生するアニメーション
	DrawAnimationList(clip, animations);
	ImGui::Separator();

	// 再生の設定
	ImGui::Checkbox("Loop", &clip.loop);
	ImGui::Checkbox("Root Motion", &clip.rootMotion);
	ImGui::Separator();

	// 出ていくリンクの一覧
	DrawOutLinkList(clip.id, false);

	// 削除
	if (StateGraphDraw::DeleteButton("Delete Clip"))
	{
		DeleteSelection(*data);
	}
}

// 複数選択時
void StudioStateChildCanvas::DrawMultiInspector()
{
	ImGui::Text("%d nodes selected", static_cast<int>(selectedNodeKeys.size()));
	ImGui::Separator();

	// まとめて削除
	if (StateGraphDraw::DeleteButton("Delete Clips"))
	{
		DeleteSelection(*data);
	}
}

// リンクの設定
void StudioStateChildCanvas::DrawLinkInspector(int linkId)
{
	EntryLink* entryLink = FindEntryLink(linkId);
	ClipLink* clipLink = FindClipLink(linkId);
	if (!entryLink && !clipLink)
	{
		ImGui::TextDisabled("Nothing selected.");
		return;
	}

	ImGui::Text("Link");
	ImGui::Separator();

	// 元 → 先 と、同じ遷移元の中で何番目か
	int index = 0;
	int count = 0;
	if (entryLink)
	{
		ImGui::Text("Entry(%s)  ->  %s",
			GetSourceName([&] { for (const auto& e : entries) if (e.transitionId == entryLink->entryTransitionId) return e.fromKey; return -1; }()).c_str(),
			GetClipName(entryLink->toClipId).c_str());

		for (const auto& l : state->child.entryLinks)
		{
			if (l.entryTransitionId != entryLink->entryTransitionId) continue;
			if (l.id == linkId) index = count;
			++count;
		}
	}
	else
	{
		ImGui::Text("%s  ->  %s", GetClipName(clipLink->fromClipId).c_str(), GetClipName(clipLink->toClipId).c_str());

		for (const auto& l : state->child.clipLinks)
		{
			if (l.fromClipId != clipLink->fromClipId) continue;
			if (l.id == linkId) index = count;
			++count;
		}
	}

	// 優先度
	ImGui::Text("Priority: %d / %d", index + 1, count);
	ImGui::Spacing();

	// 常に成立
	bool& always = entryLink ? entryLink->always : clipLink->always;
	ImGui::Checkbox("Always", &always);

	// ブレンド
	if (clipLink)
	{
		ImGui::Checkbox("Blend", &clipLink->blend.enabled);
		ImGui::BeginDisabled(!clipLink->blend.enabled);
		ImGui::DragFloat("Blend Duration", &clipLink->blend.duration, 0.01f, 0.0f, 5.0f, "%.2f s");
		ImGui::EndDisabled();
	}
	ImGui::Spacing();

	// 条件の状態
	const ConditionGraph& condition = entryLink ? entryLink->condition : clipLink->condition;
	if (always)
	{
		ImGui::TextDisabled("Condition: Always");
	}
	else if (condition.IsEmpty())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Condition: Empty");
	}
	else
	{
		ImGui::Text("Condition: Set");
	}

	// 条件の編集へ
	if (ImGui::Button("Edit Condition", ImVec2(-1.0f, 0.0f)))
	{
		ConditionTarget::Kind kind = entryLink ? ConditionTarget::Kind::EntryLink : ConditionTarget::Kind::ClipLink;
		editConditionRequest = { kind, stateId, linkId };
	}

	// 削除
	if (StateGraphDraw::DeleteButton("Delete Link"))
	{
		DeleteSelection(*data);
	}

	// 同じ階層のリンクから、設定を写すためのリスト
	ConditionTarget::Kind kind = entryLink ? ConditionTarget::Kind::EntryLink : ConditionTarget::Kind::ClipLink;
	ConditionTarget self{ kind, stateId, linkId };
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Text("Copy From");

	// 受け口に落とされたら、写す依頼を出す
	ConditionTarget source = ConditionCopyList::DrawDropTarget(self);
	if (source.IsValid()) copyRequest = { source, self };

	// 写す元のリスト
	ConditionCopyList::Draw(ConditionCopyList::CollectChild(*data, self));
}

// クリップのアニメーション一覧
void StudioStateChildCanvas::DrawAnimationList(ClipNode& clip, const std::vector<std::string>& animations)
{
	ImGui::Text("Animation");

	// 一覧を表示するクリップが変わったら、今のアニメーションの位置へスクロールする
	if (animationListClipId != clip.id)
	{
		animationListClipId = clip.id;
		scrollToCurrent = true;
	}

	// 設定されている名前が、モデルのアニメーションに無い時
	bool missing = !clip.clipName.empty() &&
		std::find(animations.begin(), animations.end(), clip.clipName) == animations.end();
	if (missing)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s (missing)", clip.clipName.c_str());
	}

	// 絞り込みの文字
	std::string filter = animationFilter;
	std::transform(filter.begin(), filter.end(), filter.begin(),
		[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

	// 一覧表示
	ImGui::BeginChild("##AnimationList", ImVec2(0.0f, 200.0f), true);
	for (const auto& name : animations)
	{
		if (!filter.empty())
		{
			std::string lower = name;
			std::transform(lower.begin(), lower.end(), lower.begin(),
				[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
			if (lower.find(filter) == std::string::npos) continue;
		}

		bool current = (name == clip.clipName);

		// クリックで確定
		if (ImGui::Selectable(name.c_str(), current))
		{
			clip.clipName = name;
		}

		// 今のアニメーションが見える位置までスクロールする
		if (current && scrollToCurrent)
		{
			ImGui::SetScrollHereY(0.5f);
		}
	}
	ImGui::EndChild();
	scrollToCurrent = false;

	// 検索欄入力
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputTextWithHint("##AnimationFilter", "Filter...", animationFilter, sizeof(animationFilter)))
	{
		// 絞り込みを変えたら、今のアニメーションが残っていればその位置へ
		scrollToCurrent = true;
	}

}

// 出ていくリンクの一覧
void StudioStateChildCanvas::DrawOutLinkList(int fromKey, bool fromEntry)
{
	ImGui::Text("Links (priority order)");

	// 入れ替えは描画が終わってから
	int moveId = -1;
	int moveOffset = 0;
	int number = 0;

	// リンク1本分
	auto drawRow = [&](int linkId, int toClipId, bool always, const ConditionGraph& condition)
		{
			ImGui::PushID(linkId);

			// 上下で優先度を入れ替える
			if (ImGui::ArrowButton("##Up", ImGuiDir_Up)) { moveId = linkId; moveOffset = -1; }
			ImGui::SameLine();
			if (ImGui::ArrowButton("##Down", ImGuiDir_Down)) { moveId = linkId; moveOffset = +1; }
			ImGui::SameLine();

			// 条件の設定漏れは赤で表示
			bool invalid = (!always && condition.IsEmpty());
			if (invalid) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));

			// クリックでそのリンクを選ぶ
			std::string label = std::to_string(++number) + ". -> " + GetClipName(toClipId);
			if (ImGui::Selectable(label.c_str(), linkId == selectedLinkId))
			{
				selectedLinkId = linkId;
				selectedNodeKeys.clear();
				clearSelectionRequested = true; // キャンバス側のノードの選択も外す
			}

			if (invalid) ImGui::PopStyleColor();
			ImGui::PopID();
		};

	// 遷移元に合うリンクを、配列の並び順に描く
	if (fromEntry)
	{
		for (const auto& l : state->child.entryLinks)
		{
			if (l.entryTransitionId == fromKey) drawRow(l.id, l.toClipId, l.always, l.condition);
		}
	}
	else
	{
		for (const auto& l : state->child.clipLinks)
		{
			if (l.fromClipId == fromKey) drawRow(l.id, l.toClipId, l.always, l.condition);
		}
	}

	if (number == 0)
	{
		ImGui::TextDisabled("No links.");
	}

	// 描画が終わってから入れ替える
	if (moveId != -1)
	{
		if (fromEntry) data->MoveEntryLink(stateId, moveId, moveOffset);
		else           data->MoveClipLink(stateId, moveId, moveOffset);
	}
}

// IDからリンクを探す
EntryLink* StudioStateChildCanvas::FindEntryLink(int linkId)
{
	for (auto& l : state->child.entryLinks)
	{
		if (l.id == linkId) return &l;
	}
	return nullptr;
}
ClipLink* StudioStateChildCanvas::FindClipLink(int linkId)
{
	for (auto& l : state->child.clipLinks)
	{
		if (l.id == linkId) return &l;
	}
	return nullptr;
}

// 遷移元のノードのキーから、表示用の名前を作る
std::string StudioStateChildCanvas::GetSourceName(int fromKey) const
{
	if (fromKey == StateMachineData::kEntryId) return "Entry";
	if (const StateNode* s = std::as_const(*data).FindState(fromKey)) return s->name;
	return "(missing)";
}

// クリップのIDから、表示用の名前を作る
std::string StudioStateChildCanvas::GetClipName(int clipId) const
{
	if (const ClipNode* c = state->child.FindClip(clipId)) return c->name;
	return "(missing)";
}

// fromClipIdからtoClipIdへのリンクがあるか
bool StudioStateChildCanvas::HasClipLink(int fromClipId, int toClipId) const
{
	return std::any_of(state->child.clipLinks.begin(), state->child.clipLinks.end(),
		[=](const ClipLink& l) { return l.fromClipId == fromClipId && l.toClipId == toClipId; });
}

// 線の始点と終点を求める
bool StudioStateChildCanvas::CalcLinkPoints(int fromKey, int toKey, bool hasReverse, int lane, ImVec2& outStart, ImVec2& outEnd) const
{
	auto fromIt = nodeRects.find(fromKey);
	auto toIt = nodeRects.find(toKey);
	if (fromIt == nodeRects.end() || toIt == nodeRects.end()) return false;

	StateGraphDraw::CalcLinkPoints(
		fromIt->second.first, fromIt->second.second, fromKey,
		toIt->second.first, toIt->second.second, toKey,
		hasReverse, lane,
		outStart, outEnd);
	return true;
}
