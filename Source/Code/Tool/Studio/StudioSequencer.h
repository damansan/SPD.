#pragma once
#include "IStudioAttachment.h"
#include "Engine/Core/JsonFileCache.h"
#include "Engine/Data/SequencerData.h"
#include "StudioTimelineWidget.h"
#include <imgui.h>
#include <string>
#include <vector>

class Model;
struct AnimatorComponent;

class StudioSequencer : public IStudioAttachment
{
public:
	StudioSequencer() = default;
	~StudioSequencer() = default;

	void SetWindowId(int id) { windowId = id; }

	// ImGui描画
	void DrawGUI(entt::registry& registry, StudioContext& context) override;

private:
	// 選んでいるものの種類
	enum class SelectionType
	{
		None,  // トラックだけを選んでいる
		Point, // Point を選んでいる
		Range, // Range を選んでいる
	};

	// == 内部状態 == //
	int windowId = 0;

	SequencerData sequencerData;    // 編集中のデータ
	JsonFileCache<SequencerData> fileCache; // ファイルの読み書き
	std::string loadedModelPath;    // 最後に読み込んだモデルパス
	bool isDirty = false;           // 未保存の変更があるか
	StudioTimelineWidget timeline;  // タイムライン部品

	SequencerFlag selectedTrack = SequencerFlag::AnimExit; // 選んでいるトラック
	SelectionType selectedType = SelectionType::None;      // 選んでいるものの種類
	int selectedId = -1;                                   // 選んでいる Point / Range の id
	std::string selectedClipName;


	// タイムライン
	bool DrawTimelineArea(const Model& model, AnimatorComponent& animator, StudioContext& context);
	// 数値
	bool DrawDetailArea(const Model& model, const std::string& clipName, const std::string& modelPath); // 右：詳細（変更があればtrue）
	// 詳細
	bool DrawSelectedItem(const Model& model, const std::string& clipName); 

	// トラックの描画
	bool DrawTrack(SequencerFlag flag, const Model& model, const std::string& clipName, float frameStep);
	// Point の行
	bool DrawPointLane(SequencerFlag flag, const Model& model, const std::string& clipName, float frameStep); 
	// Range の行
	bool DrawRangeLane(SequencerFlag flag, const Model& model, const std::string& clipName, float frameStep);
	// 閉じているときの細い行
	void DrawCollapsedLane(SequencerFlag flag, const Model& model, const std::string& clipName);

	// ボタン
	bool DrawItemButtons(const Model& model, const std::string& clipName, float pinTime, float frameStep);
	
	// フラグの表示名
	static const char* GetFlagName(SequencerFlag flag);

	// 保存 / 読み込み
	void Load(const std::string& modelPath);
	void Save(const std::string& modelPath);
}; 