#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <json.hpp>
#include "Engine/Data/ConditionGraphData.h"

// エディタ上の位置
struct EditorPosition
{
	float x = 0.0f;
	float y = 0.0f;
};

// ブレンドの設定
struct BlendSetting
{
	bool enabled = true;   // ブレンドするか
	float duration = 0.2f; // ブレンドの秒数(enabledがfalseなら使わない)
};

// 親階層 : 遷移(State → State)
struct StateTransition
{
	int id = -1;
	int toStateId = -1;
	bool always = false; // trueなら条件に関係なく成立する
	BlendSetting blend;
	ConditionGraph condition;
};

// 子階層：再生するクリップ
struct ClipNode
{
	int id = -1;
	std::string name;
	std::string clipName;
	bool loop = true;
	bool rootMotion = false;
	EditorPosition editorPos;
};

// 子階層：遷移(エントリー → クリップ)
struct EntryLink
{
	int id = -1;
	int entryTransitionId = -1; // どの遷移から来たエントリーか
	int toClipId = -1;
	bool always = false;
	ConditionGraph condition;
};

// 子階層：遷移(クリップ → クリップ)
struct ClipLink
{
	int id = -1;
	int fromClipId = -1;
	int toClipId = -1;
	bool always = false;
	BlendSetting blend;
	ConditionGraph condition;
};

// 子階層(そのStateで何を再生するか)
struct ChildGraph
{
	std::vector<ClipNode>  clips;
	std::vector<EntryLink> entryLinks;
	std::vector<ClipLink>  clipLinks;

	// エントリーのエディタ上の位置
	std::unordered_map<int, EditorPosition> entryEditorPos;

	const ClipNode* FindClip(int clipId) const;
};

// State(親)
struct StateNode
{
	int id = -1;
	std::string name;
	std::vector<StateTransition> transitions; // 並び順がそのまま優先度
	ChildGraph child;

	EditorPosition editorPos;
	float editorColor[3] = { 0.27f, 0.35f, 0.47f };
};

// 親のEntry(最初のStateへの遷移を持つ)
struct StateMachineEntry
{
	std::vector<StateTransition> transitions;
	EditorPosition editorPos;
};

// Stateマシンのグラフ全体
class StateMachineData
{
public:
	static constexpr int kVersion = 2;

	StateMachineEntry entry;
	std::vector<StateNode> states;

	// ID採番
	int GenerateId() { return nextId++; }

	// 検索
	const StateNode* FindState(int stateId) const;
	const StateNode* FindStateByName(const std::string& name) const;
	const StateTransition* FindTransition(int transitionId) const; // Entryの遷移も含む

	// 遷移元にEntryを指定するときのID
	static constexpr int kEntryId = 0;

	
	
	// 空のグラフにする
	void Clear();

	// State
	int  AddState(const std::string& baseName, const EditorPosition& pos); // 作成
	bool RenameState(int stateId, const std::string& newName);             // 名前変更
	void RemoveState(int stateId);                                         // 削除

	// 遷移
	int  AddTransition(int fromStateId, int toStateId); // 作成
	void RemoveTransition(int transitionId);            // 削除
	bool MoveTransition(int transitionId, int offset);  // 優先度の入れ替え

	// 子階層：クリップ
	int  AddClip(int stateId, const std::string& name, const EditorPosition& pos); // 作成
	void RemoveClip(int stateId, int clipId);                                      // 削除

	// 子階層：(エントリー → クリップ)
	int  AddEntryLink(int stateId, int transitionId, int toClipId); // 作成
	void RemoveEntryLink(int stateId, int linkId);                  // 削除
	bool MoveEntryLink(int stateId, int linkId, int offset);        // 優先度の入れ替え

	// 子階層：(クリップ → クリップ)
	int  AddClipLink(int stateId, int fromClipId, int toClipId); // 作成
	void RemoveClipLink(int stateId, int linkId);                // 削除
	bool MoveClipLink(int stateId, int linkId, int offset);      // 優先度の入れ替え

	// 編集用の検索
	StateNode* FindState(int stateId);
	StateTransition* FindTransition(int transitionId);

	// 他のStateと重複しない名前を作る
	std::string MakeUniqueStateName(const std::string& baseName) const;



	// 整合性チェック
	bool Validate(const std::string& sourceName) const;

	// 保存/読み込み
	nlohmann::ordered_json ToJson() const;
	bool FromJson(const nlohmann::json& j, const std::string& sourceName);

	// モデルパスから保存先のパスを組み立てる
	static std::string GetFilePath(const std::string& modelPath);

private:
	int nextId = 1;

	// 遷移を持っているリスト(Entry、またはどれかのState)を探す
	std::vector<StateTransition>* FindTransitionOwner(int transitionId);
};