#include "StudioConditionCopyList.h"
#include <imgui.h>

// このファイルの中だけで使う処理
namespace
{
	// 写す元にできる条件か
	bool IsCopySource(bool always, const ConditionGraph& condition)
	{
		// alwaysのリンクは、条件を使っていないので出さない
		if (always) return false;

		// 条件が空(赤いリンク)は、写しても空になるだけなので出さない
		if (condition.IsEmpty()) return false;

		// 決まりに合わないリンクを含む条件は、エラーごと写ってしまうので出さない
		for (const auto& l : condition.links)
		{
			if (!condition.IsLinkValid(l)) return false;
		}
		return true;
	}

	// 親の遷移のIDから、遷移元の名前を作る
	std::string GetTransitionSourceName(const StateMachineData& data, int transitionId)
	{
		for (const auto& t : data.entry.transitions)
		{
			if (t.id == transitionId) return "Entry";
		}
		for (const auto& s : data.states)
		{
			for (const auto& t : s.transitions)
			{
				if (t.id == transitionId) return s.name;
			}
		}
		return "(missing)";
	}

	// StateのIDから、表示用の名前を作る
	std::string GetStateName(const StateMachineData& data, int stateId)
	{
		const StateNode* s = data.FindState(stateId);
		return s ? s->name : "(missing)";
	}

	// クリップのIDから、表示用の名前を作る
	std::string GetClipName(const ChildGraph& child, int clipId)
	{
		const ClipNode* c = child.FindClip(clipId);
		return c ? c->name : "(missing)";
	}
}

// 親の階層のリンクを集める
std::vector<ConditionCopyItem> ConditionCopyList::CollectParent(const StateMachineData& data, const ConditionTarget& self)
{
	std::vector<ConditionCopyItem> items;

	// リンク1本分を、条件に合えば足す
	auto add = [&](const std::string& fromName, const StateTransition& t)
		{
			ConditionTarget target{ ConditionTarget::Kind::StateLink, -1, t.id };
			if (target == self) return; // 自分自身は出さない
			if (!IsCopySource(t.always, t.condition)) return;

			items.push_back({ target, "", fromName + " -> " + GetStateName(data, t.toStateId) });
		};

	// Entryから出るリンク
	for (const auto& t : data.entry.transitions) add("Entry", t);

	// 各Stateから出るリンク
	for (const auto& s : data.states)
	{
		for (const auto& t : s.transitions) add(s.name, t);
	}

	return items;
}

// 子の階層のリンクを集める
std::vector<ConditionCopyItem> ConditionCopyList::CollectChild(const StateMachineData& data, const ConditionTarget& self)
{
	std::vector<ConditionCopyItem> items;

	// 全てのStateの子のリンクを、State名を見出しにして集める
	for (const auto& s : data.states)
	{
		// エントリー → クリップ
		for (const auto& l : s.child.entryLinks)
		{
			ConditionTarget target{ ConditionTarget::Kind::EntryLink, s.id, l.id };
			if (target == self) continue; // 自分自身は出さない
			if (!IsCopySource(l.always, l.condition)) continue;

			std::string label = "Entry(" + GetTransitionSourceName(data, l.entryTransitionId) + ") -> " + GetClipName(s.child, l.toClipId);
			items.push_back({ target, s.name, label });
		}

		// クリップ → クリップ
		for (const auto& l : s.child.clipLinks)
		{
			ConditionTarget target{ ConditionTarget::Kind::ClipLink, s.id, l.id };
			if (target == self) continue; // 自分自身は出さない
			if (!IsCopySource(l.always, l.condition)) continue;

			std::string label = GetClipName(s.child, l.fromClipId) + " -> " + GetClipName(s.child, l.toClipId);
			items.push_back({ target, s.name, label });
		}
	}

	return items;
}

// リストを描く
void ConditionCopyList::Draw(const std::vector<ConditionCopyItem>& items)
{
	ImGui::BeginChild("##ConditionCopyList", ImVec2(0.0f, 150.0f), true);

	if (items.empty())
	{
		ImGui::TextDisabled("No links to copy from.");
	}

	std::string previousGroup;
	for (size_t i = 0; i < items.size(); ++i)
	{
		const ConditionCopyItem& item = items[i];

		// 見出し
		bool hasGroup = !item.group.empty();
		if (hasGroup && (i == 0 || item.group != previousGroup))
		{
			ImGui::TextDisabled("%s", item.group.c_str());
		}
		previousGroup = item.group;

		// 項目
		ImGui::PushID(item.target.linkId);
		if (hasGroup) ImGui::Indent();
		ImGui::Selectable(item.label.c_str());

		// ドラッグ開始：どのリンクかを運ぶ
		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload(kPayloadType, &item.target, sizeof(ConditionTarget));
			ImGui::Text("Copy: %s", item.label.c_str());
			ImGui::EndDragDropSource();
		}

		if (hasGroup) ImGui::Unindent();
		ImGui::PopID();
	}

	ImGui::EndChild();
}

// 受け口を描く
ConditionTarget ConditionCopyList::DrawDropTarget(const ConditionTarget& self)
{
	ConditionTarget dropped;

	// 受け口の枠
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.20f, 0.25f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.25f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.20f, 0.25f, 1.0f));
	ImGui::Button("Drop a link here to copy", ImVec2(-1.0f, 40.0f));
	ImGui::PopStyleColor(3);

	// リストから運ばれてきたリンクを受け取る
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kPayloadType))
		{
			// 運ばれてきた物の大きさが合っているときだけ使う
			if (payload->DataSize == sizeof(ConditionTarget))
			{
				ConditionTarget source = *static_cast<const ConditionTarget*>(payload->Data);

				// 自分自身は写さない
				if (source != self) dropped = source;
			}
		}
		ImGui::EndDragDropTarget();
	}

	return dropped;
}