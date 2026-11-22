#pragma once
#include <unordered_map>
#include <utility>
#include <vector>
#include <string>

#include "StudioStateGraphDraw.h"
#include "StudioConditionTarget.h"
#include "StudioConditionCopyList.h"

#include "Engine/Data/StateMachineData.h"
#include "Tool/NodeEditor/NodeEditorCanvas.h"

// Stateグラフの子のキャンバス
class StudioStateChildCanvas
{
public:
	// 
	explicit StudioStateChildCanvas(int stateId) : stateId(stateId) {}

	void Initialize();

	// どのStateの子か
	int GetStateId() const { return stateId; }

	// キャンバスを描く
	void Draw(StateMachineData& data,const std::vector<std::string>& animations);
	// 選択している物を削除
	void DeleteSelection(StateMachineData& data);

	// 条件を編集してほしいリンクからの依頼
	ConditionTarget ConsumeEditConditionRequest() { ConditionTarget t = editConditionRequest; editConditionRequest = {}; return t; }
	// リンクの設定を写す依頼
	ConditionCopyRequest ConsumeCopyRequest() { ConditionCopyRequest r = copyRequest; copyRequest = {}; return r; }

private:
	// --- 描画 --- //

	// 入ってくるリンクからエントリーの一覧を作る
	void CollectEntries();

	// ノード描画
	void DrawEntryNodes();
	void DrawClipNodes();

	// リンク描画
	void DrawLinks();
	// 作成中のリンク描画
	void DrawPendingLink();

	// 背景右クリックのメニュー
	void DrawBackgroundMenu();
	// ノード右クリックのメニュー
	void DrawNodeMenu();


	// --- 入力 --- //

	// リンク作成中の入力
	void UpdateLinkCreation();
	// 選択の更新
	void UpdateSelection();



	// --- 判定 --- //

	// データに存在するノードか判定
	bool IsExistingNode(int key) const;
	// エントリーか判定
	bool IsEntry(int key) const;

	
	// --- インスペクター --- //

	// インスペクター描画
	void DrawInspector(const std::vector<std::string>& animations);
	
	// エントリーの設定
	void DrawEntryInspector(int transitionId);
	// クリップの設定
	void DrawClipInspector(ClipNode& clip, const std::vector<std::string>& animations);
	// 複数選択時
	void DrawMultiInspector();
	// リンクの設定
	void DrawLinkInspector(int linkId);

	// クリップのアニメーション一覧
	void DrawAnimationList(ClipNode& clip, const std::vector<std::string>& animations);

	// 出ていくリンクの一覧
	void DrawOutLinkList(int fromKey, bool fromEntry);

	// IDからリンクを探す
	EntryLink* FindEntryLink(int linkId);
	ClipLink* FindClipLink(int linkId);

	// 遷移元のノードのキーから、表示用の名前を作る
	std::string GetSourceName(int fromKey) const;
	// クリップのIDから、表示用の名前を作る
	std::string GetClipName(int clipId) const;


	// --- リンクの計算 --- //

	// fromClipIdからtoClipIdへのリンクがあるか
	bool HasClipLink(int fromClipId, int toClipId) const;
	// 線の始点と終点を求める
	bool CalcLinkPoints(int fromKey, int toKey, bool hasReverse, int lane, ImVec2& outStart, ImVec2& outEnd) const;
	

	// --- データ --- //

	int stateId = -1;

	StateMachineData* data = nullptr;
	StateNode* state = nullptr;

	// エントリー
	struct EntryInfo
	{
		int transitionId; // 入ってくる遷移のID
		int fromKey;      // 遷移元のノードのキー
	};
	std::vector<EntryInfo> entries;


	// --- キャンバスの状態 --- //
	
	// 条件を編集してほしいリンク
	ConditionTarget editConditionRequest;
	// 設定を写してほしいリンク
	ConditionCopyRequest copyRequest;

	NodeEditorCanvas canvas;
	// データの位置をノードに反映するか
	bool needsApplyPositions = true;
	// ノードの矩形
	std::unordered_map<int, std::pair<ImVec2, ImVec2>> nodeRects;

	// 描いたリンクの線
	struct LinkSegment
	{
		int linkId;
		ImVec2 start;
		ImVec2 end;
	};
	std::vector<LinkSegment> linkSegments;

	// 選択中のノードとLink
	std::vector<int> selectedNodeKeys;
	int selectedLinkId = -1;

	// 次にキャンバスを描くときに、ノードエディタ側の選択を消すか
	bool clearSelectionRequested = false;

	// リンクの作成中の遷移元
	int pendingFromKey = -1;

	// 背景の右クリックの位置
	ImVec2 pendingCreatePos{};
	// ノードの右クリックの対象
	int contextNodeKey = -1;

	bool columnWidthInitialized = false;

	// クリップの名前の入力欄
	int nameEditingClipId = -1;
	char nameBuffer[128] = {};

	// アニメーションの一覧
	char animationFilter[64] = {};   // 絞り込みの文字
	int animationListClipId = -1;    // 一覧を表示しているクリップ
	bool scrollToCurrent = false;
};