#pragma once
#include <d3d11.h>
#include <wrl.h>
#include "Camera.h"

// ゲームを描画するための専用テクスチャと、ゲームカメラ
class GameView
{
public:
    GameView() = default;
    ~GameView() = default;
    GameView(const GameView&) = delete;
    GameView& operator=(const GameView&) = delete;
    GameView(GameView&&) noexcept = delete;
    GameView& operator=(GameView&&) noexcept = delete;

    // 初期化
    void Initialize(ID3D11Device* device, UINT width = 1920, UINT height = 1080);

    // 描画先の切り替え
    void Begin(ID3D11DeviceContext* dc);
    void End(ID3D11DeviceContext* dc);

    // テクスチャの再生成
    void ResizeRenderTarget(UINT width, UINT height);

    // 表示先の縦横比に合わせて、ゲームカメラのプロジェクションを更新する
    void SetAspect(float aspect);

    // ゲームカメラ取得
    Camera& GetCamera() { return camera; }

    // シーン（HDR）
    ID3D11RenderTargetView* GetSceneRTV() { return rtv.Get(); }
    ID3D11ShaderResourceView* GetSceneSRV() { return srv.Get(); }
    ID3D11DepthStencilView* GetDSV() { return dsv.Get(); }

    // 表示用（8bit）
    ID3D11RenderTargetView* GetOutputRTV() { return outputRtv.Get(); }
    ID3D11ShaderResourceView* GetOutputSRV() { return outputSrv.Get(); }

    UINT GetRenderWidth()  const { return renderWidth; }
    UINT GetRenderHeight() const { return renderHeight; }

private:
    Microsoft::WRL::ComPtr<ID3D11Device>             device;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          renderTexture;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   rtv;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          depthTexture;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   dsv;

    // 表示用
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          outputTexture;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   outputRtv;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> outputSrv;

    Camera camera; // ゲームカメラ

    UINT renderWidth = 1920;
    UINT renderHeight = 1080;

    // Begin/End間でのビューポート退避用
    D3D11_VIEWPORT savedViewport = {};
};