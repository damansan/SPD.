#pragma once
#include <string>
#include <memory>
#include <vector>

#include "IStudioAttachment.h"
#include "Engine/Core/JsonFileCache.h"
#include "Engine/Data/StateMachineData.h"

#include "Tool/NodeEditor/NodeEditorCanvas.h"

#include "StudioStateParentCanvas.h"
#include "StudioStateChildCanvas.h"

#include "StudioConditionCanvas.h"
#include "StudioConditionTarget.h"
#include "StudioConditionCopyList.h"

// Stateグラフの編集パネル
class StudioStateGraph : public IStudioAttachment
{
public:
	void Initialize();
	void SetWindowId(int id) { windowId = id; }
	void DrawGUI(entt::registry& registry, StudioContext& context) override;

	// モデルが変わったときに呼ぶ
	void OnModelChanged(StudioContext& context);

private:
	// --- ファイル関係 --- //

// ツールバー
	void DrawToolbar(StudioContext& context);

	// 保存 / 読み込み（場所はモデルのパスから決まる）
	void Save(StudioContext& context);
	void Load(StudioContext& context);

	// .stategraph.json の中身
	struct GraphFile
	{
		StateMachineData data;
		bool valid = false; // 検証を通ったか
	};

	// モデルが持っているアニメーションの名前の一覧を作り直す
	void RefreshAnimationNames(entt::registry& registry);
	// アニメーションを再生できるモデルか
	static bool HasAnimatedModel(entt::registry& registry);


	// --- データ --- //

	StateMachineData data;              // 編集中のグラフ
	JsonFileCache<GraphFile> fileCache; // ファイルの読み書き

	// --- パネルの状態 --- //

	int windowId = 0;

	// 最後の検証でエラーがあったか
	bool hasErrors = false;
	// ツールバーに出すメッセージ
	std::string message;

	bool columnWidthInitialized = false;


	// --- キャンバス --- //

	// 親階層キャンバス
	StudioStateParentCanvas parentCanvas;
	// 子階層キャンバス(複数)
	std::vector<std::unique_ptr<StudioStateChildCanvas>> childCanvases;
	
	// 条件のウィンドウ
	StudioConditionCanvas conditionCanvas;      // 新しい条件のキャンバス
	ConditionTarget conditionTarget;            // 編集中の条件がどのリンクのものか

	ConditionGraph* conditionPointer = nullptr; // 前のフレームで見つけた実体
	bool focusConditionWindow = false;          // 次のフレームで手前に出すか
	bool conditionColumnInitialized = false;


	// --- 子階層のキャンバス --- //

	// 次に手前に出す子のウィンドウ
	int focusChildStateId = -1;

	// 子のウィンドウを開く
	void OpenChildWindow(int stateId);
	// 開いている子のウィンドウを描く
	void DrawChildWindows(ImGuiID ownerViewportId);

	// モデルが持っているアニメーションの名前
	std::vector<std::string> animationNames;


	// --- 条件のウィンドウ --- //

	// 条件のウィンドウを開く
	void OpenConditionWindow(const ConditionTarget& target);
	// 条件のウィンドウを描く
	void DrawConditionWindow(ImGuiID ownerViewportId);
	// 条件のウィンドウを閉じる
	void CloseConditionWindow();
	// 編集する条件の実体を探す
	ConditionGraph* ResolveCondition(const ConditionTarget& target, std::string& outTitle, ImVec4& outColor);
	// 親のリンクのIDから、遷移元の名前と色を探す
	bool FindTransitionSource(int transitionId, std::string& outName, ImVec4& outColor) const;


	// --- リンクの設定を写す --- //

	// リンクが持つ設定の場所(ブレンドを持たないリンクはnullptr)
	struct LinkSettingsRef
	{
		ConditionGraph* condition = nullptr;
		bool* always = nullptr;
		BlendSetting* blend = nullptr;
	};

	// リンクの設定の場所を探す
	bool ResolveLinkSettings(const ConditionTarget& target, LinkSettingsRef& out);
	// 写す依頼を受け取る
	void RequestCopy(const ConditionCopyRequest& request);
	// 写す
	void CopyLinkSettings(const ConditionCopyRequest& request);
	// 上書きの確認のポップアップ
	void DrawCopyConfirmPopup();

	// 確認中の依頼
	ConditionCopyRequest pendingCopy;
	// 次に確認のポップアップを開くか
	bool openCopyConfirm = false;
};