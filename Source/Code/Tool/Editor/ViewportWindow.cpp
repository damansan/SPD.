#include "ViewportWindow.h"
#include <imgui.h>
#include <imgui_internal.h>

// 初期化
void ViewportWindow::Initialize(UINT width, UINT height)
{
    debugCamera.Initialize(static_cast<float>(width), static_cast<float>(height));
}

// ImGui描画
void ViewportWindow::Draw(GameView& gameView, const std::function<void()>& drawMenus)
{
    ImGui::Begin("Game", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // メニューバー
    if (ImGui::BeginMenuBar())
    {
        if (drawMenus)
        {
            drawMenus();
        }

        ImGui::EndMenuBar();
    }

    // Game専用のDockSpace
    ImGuiID dockSpaceId = ImGui::GetID("GameDockSpace");
    ImGui::DockSpace(dockSpaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    // 初回フレームだけデフォルトレイアウトを組む
    if (firstDockSetup)
    {
        firstDockSetup = false;

        ImGui::DockBuilderRemoveNode(dockSpaceId);
        ImGui::DockBuilderAddNode(dockSpaceId, ImGuiDockNodeFlags_None);
        ImGui::DockBuilderSetNodeSize(dockSpaceId, ImVec2(1280, 720));

        ImGuiID dockMain = dockSpaceId;
        ImGuiID dockBottom = ImGui::DockBuilderSplitNode(
            dockMain, ImGuiDir_Down, dockRatioBottom, nullptr, &dockMain);
        ImGuiID dockLeft = ImGui::DockBuilderSplitNode(
            dockMain, ImGuiDir_Left, dockRatioLeft, nullptr, &dockMain);
        ImGuiID dockRight = ImGui::DockBuilderSplitNode(
            dockMain, ImGuiDir_Right, dockRatioRight, nullptr, &dockMain);
        ImGuiID dockBottomRight = ImGui::DockBuilderSplitNode(
            dockBottom, ImGuiDir_Right, dockRatioPrefab, nullptr, &dockBottom);
        ImGuiID dockLog = ImGui::DockBuilderSplitNode(
            dockBottomRight, ImGuiDir_Right, dockRatioLog, nullptr, &dockBottomRight);

        ImGui::DockBuilderDockWindow("Viewport", dockMain);
        ImGui::DockBuilderDockWindow("Hierarchy", dockLeft);
        ImGui::DockBuilderDockWindow("Inspector", dockRight);
        ImGui::DockBuilderDockWindow("Project", dockBottom);
        ImGui::DockBuilderDockWindow("Prefab", dockBottomRight);
        ImGui::DockBuilderDockWindow("Log", dockLog);
        ImGui::DockBuilderFinish(dockSpaceId);
    }


    // Viewportパネル
    ImGui::Begin("Viewport");

    ImVec2 region = ImGui::GetContentRegionAvail();
    if (region.x > 1 && region.y > 1)
    {
        // 同じViewportに描くので、両方のカメラに同じ縦横比を当てる
        float aspect = region.x / region.y;
        gameView.SetAspect(aspect);
        debugCamera.SetPerspectiveFov(
            DirectX::XMConvertToRadians(debugCamera.GetViewingAngle()),
            aspect,
            0.1f,
            1000.0f
        );
    }

    ImGui::Image(reinterpret_cast<ImTextureID>(gameView.GetOutputSRV()), region);

    // カメラ操作の開始判定はViewport単体のホバーだけを見る
    viewportHovered = ImGui::IsWindowHovered();

    ImGui::End(); // Viewport

    ImGui::End(); // Game
}