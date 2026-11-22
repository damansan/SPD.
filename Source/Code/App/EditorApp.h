#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Engine/Core/Application.h"
#include "Tool/Editor/ImGuiManager.h"
#include "Tool/Editor/DebugCameraSystem.h"

#include "Tool/Editor/EditorContext.h"
#include "Tool/Editor/ViewportWindow.h"
#include "Tool/Editor/ProjectWindow.h"
#include "Tool/Editor/HierarchyWindow.h"
#include "Tool/Editor/InspectorWindow.h"
#include "Tool/Editor/PrefabWindow.h"
#include "Tool/Editor/LogWindow.h"
#include "Tool/Studio/StudioSharedContext.h"
#include "Tool/Studio/StudioWindow.h"

#include "GameTool/MovementMonitorWindow.h"
#include "GameTool/PostProcessToolWindow.h"
#include "Tool/AdaptiveTriggerToolWindow.h"

// エディタのアプリケーション
class EditorApp : public Application
{
public:
	using Application::Application;

protected:
	void OnInitialize() override;
	void OnPreUpdate(float elapsedTime) override;
	void OnPostUpdate(float elapsedTime) override;
	void OnFrame() override;
	void OnPresent() override;
	bool OnMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) override;
	Camera& GetRenderCamera() override;
	bool IsGameInputActive() const override;
	bool IsDebugDrawAllowed() const override { return true; }
private:
	// エディタのUIを表示しているか
	bool IsEditorActive() const;

	// デバッグカメラの更新と、Debug/Gameの切り替え
	void UpdateDebugCamera(float elapsedTime);

	// Studioを開く
	void OpenStudioWindow(const std::string& assetPath = "");

	// メニューバーの中身
	void DrawMainMenu();

	// メニューが立てた、シーンの保存・読み込みリクエストを実行する
	void ProcessSceneRequest();

	ImGuiManager imguiManager;
	EditorContext editorContext;
	ViewportWindow viewportWindow;
	DebugCameraSystem debugCameraSystem;

	ProjectWindow   projectWindow;
	HierarchyWindow hierarchyWindow;
	InspectorWindow inspectorWindow;
	PrefabWindow    prefabWindow;
	LogWindow       logWindow;

	MovementMonitorWindow     movementMonitor;
	PostProcessToolWindow     postProcessTool;
	AdaptiveTriggerToolWindow adaptiveTriggerTool;

	StudioSharedContext studioShared;
	std::vector<std::unique_ptr<StudioWindow>> studioWindows;
	int nextStudioWindowId = 0;

	// Openの確認ポップアップで開くパス
	std::string pendingOpenPath;

	// F1でゲーム単体の見た目をプレビュー
	bool previewRelease = false;
};