#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <vector>
#include <memory>
#include <imgui.h>

#include "StudioCamera.h"
#include "StudioContext.h"
#include "IStudioAttachment.h"

#include "entt.hpp"
#include "Engine/ECS/Systems/RenderSystem.h"
#include "Engine/Rendering/PostProcess.h"
#include "Engine/ECS/Systems/AnimationSystem.h"
#include "Engine/ECS/Systems/AnimationSpeedSystem.h"

#include "Engine/Rendering/LightManager.h"
#include "Engine/Resource/ResourceManager.h"

#include "StudioSharedContext.h"
#include "StudioModelViewer.h"
#include "StudioAnimator.h"
#include "StudioTimeline.h"
#include "StudioSequencer.h"
#include "StudioStateGraph.h"
#include "StudioAnimationSpeed.h"
#include "StudioAnimationEdit.h"

class StudioWindow
{
public:
    void Initialize(ID3D11Device* device, StudioSharedContext* shared, UINT width = 1920, UINT height = 1080);

    // 更新
    void Update(float elapsedTime);

    // 描画
    void Render();

    // ImGui描画
    void DrawWindow(const char* windowName = "Studio");

    // contextへのアクセス
    StudioContext& GetContext() { return context; }

    // アセット読み込み
    void LoadAsset(const std::string& assetPath);

    // 削除リクエスト
    bool IsCloseRequested() const { return closeRequested; }
    void SetWindowId(int id) 
    {
        windowId = id; 
        studioAnimator.SetWindowId(id);
        studioTimeline.SetWindowId(id);
        studioSequencer.SetWindowId(id);
        stateGraph.SetWindowId(id);
        studioAnimationSpeed.SetWindowId(id);
        studioAnimationEdit.SetWindowId(id);
    }
    int  GetWindowId()  const { return windowId; }

private:
    // 描画先の切り替え
    void Begin();
    void End();
    void ResizeRenderTarget();

    // 最大化ボタン
    void DrawMaximizeButton();

    Microsoft::WRL::ComPtr<ID3D11Texture2D>          renderTexture;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   rtv;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          depthTexture;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   dsv;

    // 表示用
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          outputTexture;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   outputRtv;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> outputSrv;

    // カメラ
    StudioCamera camera;

    // アタッチメント間共有データ
    StudioContext context;

    entt::registry    studioRegistry;
    RenderSystem      renderSystem;
    PostProcess       postProcess;
    AnimationSystem   animationSystem;
    AnimationSpeedSystem  animationSpeedSystem;
    LightManager      lightManager;
    
    StudioSharedContext* shared = nullptr;

    // 機能
    StudioModelViewer stadoiModelViewer;
    StudioAnimator    studioAnimator;
    StudioTimeline    studioTimeline;
    StudioSequencer   studioSequencer;
    StudioStateGraph  stateGraph;
    StudioAnimationSpeed studioAnimationSpeed;
    StudioAnimationEdit  studioAnimationEdit;

    // ウィンドウ状態
    int   windowId = 0;
    UINT  renderWidth = 1920;
    UINT  renderHeight = 1080;
    bool  isHovered = false;
    bool  viewportHovered = false;
    bool  closeRequested = false;
    bool  isFirstFrame = true;
    bool  firstDockSetup = true;

    // ビューポート保存
    D3D11_VIEWPORT savedViewport = {};
    int            renderSkipCount = 0;



    // 最大化
    bool   maximized = false;        // 最大化しているか
    bool   applyWindowRect = false;  // 次のフレームで位置と大きさを変えるか
    ImVec2 applyPos{};               // 変える先の位置(最大化中は、最大化した位置)
    ImVec2 applySize{};              // 変える先の大きさ(最大化中は、最大化した大きさ)
    ImVec2 restorePos{};             // 元に戻す位置
    ImVec2 restoreSize{};            // 元に戻す大きさ
};