#pragma once
#include "IStudioAttachment.h"
#include "Engine/Data/AnimationSpeedData.h"
#include <imgui.h>
#include <functional>
#include <string>
#include <vector>

class StudioAnimationSpeed : public IStudioAttachment
{
public:
	StudioAnimationSpeed() = default;
	~StudioAnimationSpeed() = default;

	void SetWindowId(int id) { windowId = id; }

	// ImGui描画
	void DrawGUI(entt::registry& registry, StudioContext& context) override;

private:
	// == 内部状態 == //
	int windowId = 0;

	AnimationSpeedData animationSpeedData;        // 編集中の速度カーブデータ
	std::string selectedClipName;                 // 今編集しているクリップ名
	std::vector<std::string> availableAnimations; // モデルが持つアニメーション名一覧

	int selectedPointId = -1;     // 選択中のPointのID
	std::string loadedModelPath;  // 最後に読み込んだモデルパス

	// == グラフ描画 == //
	void DrawToolbar(AnimationSpeedClipData& clip);                                       // ツールバー
	void DrawSpeedGraph(AnimationSpeedClipData& clip, float previewTime, float clipLength); // 速度カーブのグラフ描画
	void DrawDetailPanel(AnimationSpeedClipData& clip, const std::string& modelPath);       // 詳細パネル

	// == 削除 == //
	void DeletePoint(AnimationSpeedClipData& clip, int id); // Point削除(isEndpointは削除不可)
	void DrawDeleteButton(const char* label, const std::function<void()>& onDelete); // 削除ボタン

	// == 座標変換ヘルパー == //
	static float TimeToNormalized(float time, float clipLength);       // 秒数を0.0～1.0に
	static float NormalizedToTime(float normalized, float clipLength); // 0.0～1.0を秒数に

	// == 保存 == //
	void Save(const std::string& modelPath); // 保存
	static std::string GetAnimationSpeedFilePath(const std::string& modelPath); // モデルパスから保存先のパスを組み立てる

	// == その他 == //
	void RefreshAvailableAnimations(entt::registry& registry); // アニメーション一覧を更新する
};