#include "StateMachineData.h"
#include <algorithm>
#include <filesystem>
#include <unordered_set>
#include <utility>

#include "Engine/Core/Log.h"
#include "Engine/Data/DataFileKind.h"

namespace
{
	// エディタ上の位置
	nlohmann::ordered_json ToJson(const EditorPosition& p)
	{
		nlohmann::ordered_json j;
		j["x"] = p.x;
		j["y"] = p.y;
		return j;
	}
	EditorPosition PositionFromJson(const nlohmann::json& j)
	{
		EditorPosition p;
		p.x = j.value("x", 0.0f);
		p.y = j.value("y", 0.0f);
		return p;
	}

	// 遷移・つながりの共通部分(always / condition)
	void WriteCommon(nlohmann::ordered_json& j, bool always, const ConditionGraph& condition)
	{
		j["always"] = always;
		j["condition"] = ConditionGraphToJson(condition);
	}
	void ReadCommon(const nlohmann::json& j, bool& always, ConditionGraph& condition)
	{
		always = j.value("always", false);
		if (j.contains("condition")) ConditionGraphFromJson(condition, j["condition"]);
	}

	// ブレンドの設定
	void WriteBlend(nlohmann::ordered_json& j, const BlendSetting& blend)
	{
		j["blend"] = blend.enabled;
		j["blendDuration"] = blend.duration;
	}
	void ReadBlend(const nlohmann::json& j, BlendSetting& blend)
	{
		blend.enabled = j.value("blend", true);
		blend.duration = j.value("blendDuration", 0.2f);
	}

	// 遷移
	nlohmann::ordered_json ToJson(const StateTransition& t)
	{
		nlohmann::ordered_json j;
		j["id"] = t.id;
		j["to"] = t.toStateId;
		WriteCommon(j, t.always, t.condition);
		WriteBlend(j, t.blend);
		return j;
	}
	StateTransition TransitionFromJson(const nlohmann::json& j)
	{
		StateTransition t;
		t.id = j.value("id", -1);
		t.toStateId = j.value("to", -1);
		ReadCommon(j, t.always, t.condition);
		ReadBlend(j, t.blend);
		return t;
	}

	// 同じグループの中で隣のものと入れ替える
	template<typename T, typename GroupFn>
	bool MoveInGroup(std::vector<T>& list, int linkId, int offset, GroupFn groupOf)
	{
		int index = -1;
		for (int i = 0; i < static_cast<int>(list.size()); ++i)
		{
			if (list[i].id == linkId) index = i;
		}
		if (index < 0 || offset == 0) return false;

		// offsetの向きに、同じグループの要素を探して入れ替える
		int group = groupOf(list[index]);
		int step = (offset < 0) ? -1 : 1;
		for (int i = index + step; i >= 0 && i < static_cast<int>(list.size()); i += step)
		{
			if (groupOf(list[i]) == group)
			{
				std::swap(list[index], list[i]);
				return true;
			}
		}
		return false; // 端なので動かせない
	}
}

// 検索 
const ClipNode* ChildGraph::FindClip(int clipId) const
{
	for (const auto& c : clips)
	{
		if (c.id == clipId) return &c;
	}
	return nullptr;
}

const StateNode* StateMachineData::FindState(int stateId) const
{
	for (const auto& s : states)
	{
		if (s.id == stateId) return &s;
	}
	return nullptr;
}

const StateNode* StateMachineData::FindStateByName(const std::string& name) const
{
	for (const auto& s : states)
	{
		if (s.name == name) return &s;
	}
	return nullptr;
}

const StateTransition* StateMachineData::FindTransition(int transitionId) const
{
	for (const auto& t : entry.transitions)
	{
		if (t.id == transitionId) return &t;
	}
	for (const auto& s : states)
	{
		for (const auto& t : s.transitions)
		{
			if (t.id == transitionId) return &t;
		}
	}
	return nullptr;
}



// 空のグラフにする
void StateMachineData::Clear()
{
	entry = StateMachineEntry{};
	entry.editorPos = { 50.0f, 200.0f };
	states.clear();
	nextId = 1;
}


// State
int StateMachineData::AddState(const std::string& baseName, const EditorPosition& pos)
{
	StateNode s;
	s.id = GenerateId();
	s.name = MakeUniqueStateName(baseName);
	s.editorPos = pos;
	states.push_back(s);
	return s.id;
}
bool StateMachineData::RenameState(int stateId, const std::string& newName)
{
	StateNode* s = FindState(stateId);
	if (!s || newName.empty()) return false;
	if (s->name == newName) return true;

	// 他のStateと重複する名前は付けられない
	if (FindStateByName(newName)) return false;

	s->name = newName;
	return true;
}
void StateMachineData::RemoveState(int stateId)
{
	// 消える遷移：このStateから出る遷移と、このStateに入ってくる遷移
	std::vector<int> removedTransitions;
	for (const auto& t : entry.transitions)
	{
		if (t.toStateId == stateId) removedTransitions.push_back(t.id);
	}
	for (const auto& s : states)
	{
		for (const auto& t : s.transitions)
		{
			if (s.id == stateId || t.toStateId == stateId) removedTransitions.push_back(t.id);
		}
	}

	// 遷移の削除(行き先のエントリーも一緒に消える)
	for (int transitionId : removedTransitions)
	{
		RemoveTransition(transitionId);
	}

	// State本体の削除
	states.erase(
		std::remove_if(states.begin(), states.end(),
			[stateId](const StateNode& s) { return s.id == stateId; }),
		states.end());
}

// 遷移
int StateMachineData::AddTransition(int fromStateId, int toStateId)
{
	if (!FindState(toStateId)) return -1;

	std::vector<StateTransition>* owner = nullptr;
	if (fromStateId == kEntryId)
	{
		owner = &entry.transitions;
	}
	else if (StateNode* from = FindState(fromStateId))
	{
		owner = &from->transitions;
	}
	if (!owner) return -1;

	StateTransition t;
	t.id = GenerateId();
	t.toStateId = toStateId;
	owner->push_back(t); // 最後に追加(優先度は一番下)

	// 行き先のStateには、この遷移のエントリーが自動で存在する
	return t.id;
}
void StateMachineData::RemoveTransition(int transitionId)
{
	std::vector<StateTransition>* owner = FindTransitionOwner(transitionId);
	if (!owner) return;

	// 行き先のStateから、この遷移のエントリーに関するものを消す
	int toStateId = -1;
	for (const auto& t : *owner)
	{
		if (t.id == transitionId) toStateId = t.toStateId;
	}
	if (StateNode* to = FindState(toStateId))
	{
		auto& links = to->child.entryLinks;
		links.erase(
			std::remove_if(links.begin(), links.end(),
				[transitionId](const EntryLink& l) { return l.entryTransitionId == transitionId; }),
			links.end());
		to->child.entryEditorPos.erase(transitionId);
	}

	// 遷移本体の削除
	owner->erase(
		std::remove_if(owner->begin(), owner->end(),
			[transitionId](const StateTransition& t) { return t.id == transitionId; }),
		owner->end());
}
bool StateMachineData::MoveTransition(int transitionId, int offset)
{
	std::vector<StateTransition>* owner = FindTransitionOwner(transitionId);
	if (!owner) return false;

	int index = -1;
	for (int i = 0; i < static_cast<int>(owner->size()); ++i)
	{
		if ((*owner)[i].id == transitionId) index = i;
	}

	int newIndex = index + offset;
	if (index < 0 || newIndex < 0 || newIndex >= static_cast<int>(owner->size())) return false;

	std::swap((*owner)[index], (*owner)[newIndex]);
	return true;
}

// 子階層：クリップ
int StateMachineData::AddClip(int stateId, const std::string& name, const EditorPosition& pos)
{
	StateNode* s = FindState(stateId);
	if (!s) return -1;

	ClipNode c;
	c.id = GenerateId();
	c.name = name;
	c.editorPos = pos;
	s->child.clips.push_back(c);
	return c.id;
}
void StateMachineData::RemoveClip(int stateId, int clipId)
{
	StateNode* s = FindState(stateId);
	if (!s) return;

	// このクリップにつながっていたものを消す
	auto& entryLinks = s->child.entryLinks;
	entryLinks.erase(
		std::remove_if(entryLinks.begin(), entryLinks.end(),
			[clipId](const EntryLink& l) { return l.toClipId == clipId; }),
		entryLinks.end());

	auto& clipLinks = s->child.clipLinks;
	clipLinks.erase(
		std::remove_if(clipLinks.begin(), clipLinks.end(),
			[clipId](const ClipLink& l) { return l.fromClipId == clipId || l.toClipId == clipId; }),
		clipLinks.end());

	// クリップ本体の削除
	auto& clips = s->child.clips;
	clips.erase(
		std::remove_if(clips.begin(), clips.end(),
			[clipId](const ClipNode& c) { return c.id == clipId; }),
		clips.end());
}

// 子階層：(エントリー → クリップ)
int StateMachineData::AddEntryLink(int stateId, int transitionId, int toClipId)
{
	StateNode* s = FindState(stateId);
	if (!s || !s->child.FindClip(toClipId)) return -1;

	// このStateに入ってくる遷移でなければ、エントリーは存在しない
	const StateTransition* t = FindTransition(transitionId);
	if (!t || t->toStateId != stateId) return -1;

	EntryLink l;
	l.id = GenerateId();
	l.entryTransitionId = transitionId;
	l.toClipId = toClipId;
	s->child.entryLinks.push_back(l);
	return l.id;
}
void StateMachineData::RemoveEntryLink(int stateId, int linkId)
{
	StateNode* s = FindState(stateId);
	if (!s) return;

	auto& links = s->child.entryLinks;
	links.erase(
		std::remove_if(links.begin(), links.end(),
			[linkId](const EntryLink& l) { return l.id == linkId; }),
		links.end());
}

// 優先度の入れ替え
bool StateMachineData::MoveEntryLink(int stateId, int linkId, int offset)
{
	StateNode* s = FindState(stateId);
	if (!s) return false;

	return MoveInGroup(s->child.entryLinks, linkId, offset,
		[](const EntryLink& l) { return l.entryTransitionId; });
}

// 子階層：(クリップ → クリップ)
int StateMachineData::AddClipLink(int stateId, int fromClipId, int toClipId)
{
	StateNode* s = FindState(stateId);
	if (!s) return -1;
	if (fromClipId == toClipId) return -1; // 自分自身へのつながりは作らない
	if (!s->child.FindClip(fromClipId) || !s->child.FindClip(toClipId)) return -1;

	ClipLink l;
	l.id = GenerateId();
	l.fromClipId = fromClipId;
	l.toClipId = toClipId;
	s->child.clipLinks.push_back(l);
	return l.id;
}
void StateMachineData::RemoveClipLink(int stateId, int linkId)
{
	StateNode* s = FindState(stateId);
	if (!s) return;

	auto& links = s->child.clipLinks;
	links.erase(
		std::remove_if(links.begin(), links.end(),
			[linkId](const ClipLink& l) { return l.id == linkId; }),
		links.end());
}

// 優先度の入れ替え
bool StateMachineData::MoveClipLink(int stateId, int linkId, int offset)
{
	StateNode* s = FindState(stateId);
	if (!s) return false;

	return MoveInGroup(s->child.clipLinks, linkId, offset,
		[](const ClipLink& l) { return l.fromClipId; });
}



// 整合性チェック
bool StateMachineData::Validate(const std::string& sourceName) const
{
	int errorCount = 0;

	auto error = [&](const std::string& message)
		{
			Log("[StateMachine] エラー(%s): %s", LogPath(sourceName).c_str(), message.c_str());
			++errorCount;
		};
	auto warning = [&](const std::string& message)
		{
			Log("[StateMachine] 警告(%s): %s", LogPath(sourceName).c_str(), message.c_str());
		};

	// IDの重複
	std::unordered_set<int> usedIds;
	auto checkId = [&](int id, const std::string& what)
		{
			if (id < 0)
			{
				error(what + " のIDが設定されていません");
			}
			else if (!usedIds.insert(id).second)
			{
				error(what + " のID " + std::to_string(id) + " が重複しています");
			}
		};

	for (const auto& t : entry.transitions) checkId(t.id, "Entryの遷移");
	for (const auto& s : states)
	{
		checkId(s.id, "State「" + s.name + "」");
		for (const auto& t : s.transitions)        checkId(t.id, "「" + s.name + "」の遷移");
		for (const auto& c : s.child.clips)        checkId(c.id, "「" + s.name + "」のクリップ");
		for (const auto& l : s.child.entryLinks)   checkId(l.id, "「" + s.name + "」のエントリーのつながり");
		for (const auto& l : s.child.clipLinks)    checkId(l.id, "「" + s.name + "」のクリップのつながり");
	}

	// Stateの名前(実行側は名前で参照するので、空と重複を禁止)
	std::unordered_set<std::string> usedNames;
	for (const auto& s : states)
	{
		if (s.name.empty())
		{
			error("名前の無いStateがあります(ID " + std::to_string(s.id) + ")");
		}
		else if (!usedNames.insert(s.name).second)
		{
			error("State名「" + s.name + "」が重複しています");
		}
	}

	// Entry
	if (entry.transitions.empty())
	{
		error("Entryから出る遷移がありません");
	}

	// 遷移・つながりの共通チェック
	auto checkCommon = [&](bool always, const ConditionGraph& condition, const std::string& what)
		{
			if (!always && condition.IsEmpty())
			{
				error(what + " の条件が空です(常に成立させる場合はalwaysを指定)");
			}

			// Getterが読むフィールドが、今のコードにあるか(名前を変えた・消したときに気づけるように)
			for (const auto& g : condition.getterNodes)
			{
				if (g.componentName.empty() || g.fieldName.empty()) continue; // 未設定はエディタで表示する
				if (!Reflection::FindField(g.componentName, g.fieldName))
				{
					error(what + " の条件が、存在しないフィールド「" + g.componentName + "." + g.fieldName + "」を読んでいます");
				}
			}

			// 決まりに合わないリンク(エディタで赤く表示されるもの)
			for (const auto& l : condition.links)
			{
				std::string reason;
				if (!condition.IsLinkValid(l, &reason))
				{
					error(what + " の条件に、決まりに合わないリンクがあります(リンクID " + std::to_string(l.id) + "): " + reason);
				}
			}
		};

	auto checkBlend = [&](const BlendSetting& blend, const std::string& what)
		{
			if (blend.enabled && blend.duration <= 0.0f)
			{
				warning(what + " のブレンド時間が0以下です(ブレンドしない扱いになります)");
			}
		};

	auto checkTransition = [&](const StateTransition& t, const std::string& fromName)
		{
			std::string what = fromName + " の遷移(ID " + std::to_string(t.id) + ")";
			if (!FindState(t.toStateId))
			{
				error(what + " の行き先のStateがありません");
			}
			checkCommon(t.always, t.condition, what);
			checkBlend(t.blend, what);
		};

	for (const auto& t : entry.transitions) checkTransition(t, "Entry");
	for (const auto& s : states)
	{
		for (const auto& t : s.transitions) checkTransition(t, "「" + s.name + "」");
	}

	// 子グラフ
	for (const auto& s : states)
	{
		// このStateに入ってくる遷移のID
		std::vector<int> incoming;
		for (const auto& t : entry.transitions)
		{
			if (t.toStateId == s.id) incoming.push_back(t.id);
		}
		for (const auto& from : states)
		{
			for (const auto& t : from.transitions)
			{
				if (t.toStateId == s.id) incoming.push_back(t.id);
			}
		}

		// エントリー → クリップ
		for (const auto& l : s.child.entryLinks)
		{
			std::string what = "「" + s.name + "」のエントリーのつながり(ID " + std::to_string(l.id) + ")";
			if (std::find(incoming.begin(), incoming.end(), l.entryTransitionId) == incoming.end())
			{
				error(what + " に対応する、入ってくる遷移がありません");
			}
			if (!s.child.FindClip(l.toClipId))
			{
				error(what + " の行き先のクリップがありません");
			}
			checkCommon(l.always, l.condition, what);
		}

		// つながりの無いエントリー
		for (int transitionId : incoming)
		{
			bool connected = std::any_of(s.child.entryLinks.begin(), s.child.entryLinks.end(),
				[transitionId](const EntryLink& l) { return l.entryTransitionId == transitionId; });
			if (!connected)
			{
				error("「" + s.name + "」に、どのクリップにもつながっていないエントリーがあります(遷移ID " + std::to_string(transitionId) + ")");
			}
		}

		// クリップ → クリップ
		for (const auto& l : s.child.clipLinks)
		{
			std::string what = "「" + s.name + "」のクリップのつながり(ID " + std::to_string(l.id) + ")";
			if (!s.child.FindClip(l.fromClipId))
			{
				error(what + " の元のクリップがありません");
			}
			if (!s.child.FindClip(l.toClipId))
			{
				error(what + " の行き先のクリップがありません");
			}
			checkCommon(l.always, l.condition, what);
			checkBlend(l.blend, what);
		}
	}

	return errorCount == 0;
}

// JSON
nlohmann::ordered_json StateMachineData::ToJson() const
{
	nlohmann::ordered_json j;
	j["version"] = kVersion;
	j["nextId"] = nextId;

	// Entry
	nlohmann::ordered_json je;
	je["transitions"] = nlohmann::json::array();
	for (const auto& t : entry.transitions) je["transitions"].push_back(::ToJson(t));
	je["editor"] = ::ToJson(entry.editorPos);
	j["entry"] = je;

	// States
	j["states"] = nlohmann::json::array();
	for (const auto& s : states)
	{
		nlohmann::ordered_json js;
		js["id"] = s.id;
		js["name"] = s.name;

		js["transitions"] = nlohmann::json::array();
		for (const auto& t : s.transitions) js["transitions"].push_back(::ToJson(t));

		// 子グラフ
		nlohmann::ordered_json jc;
		jc["clips"] = nlohmann::json::array();
		for (const auto& c : s.child.clips)
		{
			nlohmann::ordered_json jclip;
			jclip["id"] = c.id;
			jclip["name"] = c.name;
			jclip["clip"] = c.clipName;
			jclip["loop"] = c.loop;
			jclip["rootMotion"] = c.rootMotion;
			jclip["editor"] = ::ToJson(c.editorPos);
			jc["clips"].push_back(jclip);
		}

		jc["entryLinks"] = nlohmann::json::array();
		for (const auto& l : s.child.entryLinks)
		{
			nlohmann::ordered_json jl;
			jl["id"] = l.id;
			jl["entry"] = l.entryTransitionId;
			jl["to"] = l.toClipId;
			WriteCommon(jl, l.always, l.condition);
			jc["entryLinks"].push_back(jl);
		}

		jc["clipLinks"] = nlohmann::json::array();
		for (const auto& l : s.child.clipLinks)
		{
			nlohmann::ordered_json jl;
			jl["id"] = l.id;
			jl["from"] = l.fromClipId;
			jl["to"] = l.toClipId;
			WriteCommon(jl, l.always, l.condition);
			WriteBlend(jl, l.blend);
			jc["clipLinks"].push_back(jl);
		}

		// エントリーのエディタ上の位置 (キーは、入ってくる遷移のID)
		nlohmann::ordered_json jentries = nlohmann::json::array();
		for (const auto& [transitionId, pos] : s.child.entryEditorPos)
		{
			nlohmann::ordered_json jp = ::ToJson(pos);
			jp["entry"] = transitionId;
			jentries.push_back(jp);
		}
		jc["editor"]["entries"] = jentries;

		js["child"] = jc;

		js["editor"] = ::ToJson(s.editorPos);
		js["editor"]["color"] = { s.editorColor[0], s.editorColor[1], s.editorColor[2] };

		j["states"].push_back(js);
	}

	return j;
}

bool StateMachineData::FromJson(const nlohmann::json& j, const std::string& sourceName)
{
	entry = StateMachineEntry{};
	states.clear();
	nextId = 1;

	int version = j.value("version", 0);
	if (version != kVersion)
	{
		Log("[StateMachine] エラー(%s): 対応していない形式（version %d）", LogPath(sourceName).c_str(), version);		
		return false;
	}

	// Entry
	if (j.contains("entry"))
	{
		const auto& je = j["entry"];
		if (je.contains("transitions"))
		{
			for (const auto& jt : je["transitions"]) entry.transitions.push_back(TransitionFromJson(jt));
		}
		if (je.contains("editor")) entry.editorPos = PositionFromJson(je["editor"]);
	}

	// States
	if (j.contains("states"))
	{
		for (const auto& js : j["states"])
		{
			StateNode s;
			s.id = js.value("id", -1);
			s.name = js.value("name", "");

			if (js.contains("transitions"))
			{
				for (const auto& jt : js["transitions"]) s.transitions.push_back(TransitionFromJson(jt));
			}

			if (js.contains("child"))
			{
				const auto& jc = js["child"];

				if (jc.contains("clips"))
				{
					for (const auto& jclip : jc["clips"])
					{
						ClipNode c;
						c.id = jclip.value("id", -1);
						c.name = jclip.value("name", "");
						c.clipName = jclip.value("clip", "");
						c.loop = jclip.value("loop", true);
						c.rootMotion = jclip.value("rootMotion", false);
						if (jclip.contains("editor")) c.editorPos = PositionFromJson(jclip["editor"]);
						s.child.clips.push_back(c);
					}
				}

				if (jc.contains("entryLinks"))
				{
					for (const auto& jl : jc["entryLinks"])
					{
						EntryLink l;
						l.id = jl.value("id", -1);
						l.entryTransitionId = jl.value("entry", -1);
						l.toClipId = jl.value("to", -1);
						ReadCommon(jl, l.always, l.condition);
						s.child.entryLinks.push_back(l);
					}
				}

				if (jc.contains("clipLinks"))
				{
					for (const auto& jl : jc["clipLinks"])
					{
						ClipLink l;
						l.id = jl.value("id", -1);
						l.fromClipId = jl.value("from", -1);
						l.toClipId = jl.value("to", -1);
						ReadCommon(jl, l.always, l.condition);
						ReadBlend(jl, l.blend);
						s.child.clipLinks.push_back(l);
					}
				}

				if (jc.contains("editor") && jc["editor"].contains("entries"))
				{
					for (const auto& jp : jc["editor"]["entries"])
					{
						s.child.entryEditorPos[jp.value("entry", -1)] = PositionFromJson(jp);
					}
				}
			}

			if (js.contains("editor"))
			{
				const auto& jed = js["editor"];
				s.editorPos = PositionFromJson(jed);
				if (jed.contains("color") && jed["color"].is_array() && jed["color"].size() == 3)
				{
					s.editorColor[0] = jed["color"][0].get<float>();
					s.editorColor[1] = jed["color"][1].get<float>();
					s.editorColor[2] = jed["color"][2].get<float>();
				}
			}

			states.push_back(s);
		}
	}

	// 採番は、ファイルの値と、実際に使われている最大IDの大きいほうに合わせる
	int maxId = 0;
	for (const auto& t : entry.transitions) maxId = (std::max)(maxId, t.id);
	for (const auto& s : states)
	{
		maxId = (std::max)(maxId, s.id);
		for (const auto& t : s.transitions)      maxId = (std::max)(maxId, t.id);
		for (const auto& c : s.child.clips)      maxId = (std::max)(maxId, c.id);
		for (const auto& l : s.child.entryLinks) maxId = (std::max)(maxId, l.id);
		for (const auto& l : s.child.clipLinks)  maxId = (std::max)(maxId, l.id);
	}
	nextId = (std::max)(j.value("nextId", 1), maxId + 1);

	return Validate(sourceName);
}

// モデルパスから保存先のパスを組み立てる
std::string StateMachineData::GetFilePath(const std::string& modelPath)
{
	return GetModelDataFilePath(modelPath, DataFileKind::StateGraph);
}

// 編集用の検索
StateNode* StateMachineData::FindState(int stateId)
{
	return const_cast<StateNode*>(std::as_const(*this).FindState(stateId));
}
StateTransition* StateMachineData::FindTransition(int transitionId)
{
	return const_cast<StateTransition*>(std::as_const(*this).FindTransition(transitionId));
}

// 他のStateと重複しない名前を作る
std::string StateMachineData::MakeUniqueStateName(const std::string& baseName) const
{
	std::string base = baseName.empty() ? "State" : baseName;
	if (!FindStateByName(base)) return base;

	// 同じ名前があれば、末尾に番号を付ける("New State 1"、"New State 2"…)
	for (int i = 1; ; ++i)
	{
		std::string name = base + " " + std::to_string(i);
		if (!FindStateByName(name)) return name;
	}
}

// 遷移を持っているリスト(Entry、またはどれかのState)を探す
std::vector<StateTransition>* StateMachineData::FindTransitionOwner(int transitionId)
{
	for (const auto& t : entry.transitions)
	{
		if (t.id == transitionId) return &entry.transitions;
	}
	for (auto& s : states)
	{
		for (const auto& t : s.transitions)
		{
			if (t.id == transitionId) return &s.transitions;
		}
	}
	return nullptr;
}
