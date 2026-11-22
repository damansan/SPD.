#pragma once
#include <functional>
#include "Engine/Core/Camera.h"
#include "Engine/Core/GameView.h"

// エディタのGameウィンドウ(Viewportとドッキングレイアウト)と、デバッグカメラ
class ViewportWindow
{
public:
    // 初期化
    void Initialize(UINT width = 1920, UINT height = 1080);

    // ImGui描画(GameViewの描画結果をViewportに表示する)
    void Draw(GameView& gameView, const std::function<void()>& drawMenus = nullptr);

    // デバッグカメラ取得
    Camera& GetDebugCamera() { return debugCamera; }

    // ビューポートがホバーされているか
    bool IsViewportHovered() const { return viewportHovered; }

private:
    Camera debugCamera; // デバッグカメラ

    bool viewportHovered = false;

    // trueの間、次のDraw()でDockSpaceのレイアウトを（再）構築する
    bool firstDockSetup = true;

    // ドッキング比率
    float dockRatioLeft = 0.1f;   // Hierarchy
    float dockRatioRight = 0.15f; // Inspector

    float dockRatioBottom = 0.2f;  // Project / Prefab
    float dockRatioPrefab = 0.75f; // Prefab / Log
    float dockRatioLog = 0.5f;     // Log
};