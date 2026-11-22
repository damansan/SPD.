#pragma once
#include <unordered_map>
#include <utility>
#include <vector>

#include "StudioStateGraphDraw.h"
#include "StudioConditionTarget.h"
#include "StudioConditionCopyList.h"

#include "Engine/Data/StateMachineData.h"
#include "Tool/NodeEditor/NodeEditorCanvas.h"

// Stateグラフの親のキャンバス
class StudioStateParentCanvas
{
public:
	void Initialize();

	// キャンバスを描く
	void Draw(StateMachineData& data);
	// 選択している物を削除
	void DeleteSelection(StateMachineData& data);
	// 読み込み・新規作成のときに、選択などを戻す
	void Reset();

	// インスペクター描画
	void DrawInspector(StateMachineData& data);

	// 選択中のノードとLink
	const std::vector<int>& GetSelectedNodeKeys() const { return selectedNodeKeys; }
	int GetSelectedLinkId() const { return selectedLinkId; }

	// 子のキャンバスを開く依頼
	int ConsumeOpenChildRequest() { int id = openChildRequest; openChildRequest = -1; return id; }
	// 条件編集するキャンバスを開く依頼
	ConditionTarget ConsumeEditConditionRequest() { ConditionTarget t = editConditionRequest; editConditionRequest = {}; return t; }
	// リンクの設定を写す依頼
	ConditionCopyRequest ConsumeCopyRequest() { ConditionCopyRequest r = copyRequest; copyRequest = {}; return r; }

private:
	// --- 描画 --- //

	// ノード描画
	void DrawEntryNode();
	void DrawStateNodes();

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

	// データに存在するノードか
	bool IsExistingNode(int key) const;


	// --- インスペクター --- //

	// ステートの設定
	void DrawStateInspector(StateNode& state);
	// エントリーの設定
	void DrawEntryInspector();
	// 複数ステートの選択時
	void DrawMultiStateInspector();
	// リンクの設定
	void DrawLinkInspector(int linkId);

	// 出ていくリンクの一覧
	void DrawLinkList(int fromKey, std::vector<StateTransition>& links);

	// リンクの遷移元と、遷移元の中での何番目かを探す
	bool FindLinkOwner(int linkId, int& outFromKey, int& outIndex, int& outCount) const;

	// ノードのキーから、表示用の名前を作る
	std::string GetNodeName(int key) const;

	// --- リンクの計算 --- //

	// fromKeyからtoStateIdへのリンクがあるか
	bool HasLink(int fromKey, int toStateId) const;
	// 線の始点と終点を求める
	bool CalcLinkPoints(int fromKey, int toStateId, int lane, ImVec2& outStart, ImVec2& outEnd) const;


	// --- ノードのID --- //

	// EntryはデータのIDと被らない範囲の値を使う
	static constexpr int64_t kEntryNodeId = 1ll << 40;
	// ノードのキー <-> ノードエディタのID
	static ed::NodeId ToNodeId(int key);
	static int ToNodeKey(ed::NodeId nodeId);


	// --- データ --- //

	StateMachineData* data = nullptr;

	// --- キャンバスの状態 --- //

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

	// 名前の入力欄
	int nameEditingStateId = -1;
	char nameBuffer[128] = {};
	// 名前を確定できなかった理由
	std::string nameError;

	// 子のウィンドウを開いてほしいState
	int openChildRequest = -1;

	// 条件を編集してほしいリンク
	ConditionTarget editConditionRequest;
	// 設定を写してほしいリンク
	ConditionCopyRequest copyRequest;
};