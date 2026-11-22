#include "GameView.h"
#include <cassert>

// 初期化
void GameView::Initialize(ID3D11Device* dev, UINT width, UINT height)
{
    device = dev;

    // カメラ初期化
    camera.Initialize(static_cast<float>(width), static_cast<float>(height));

    ResizeRenderTarget(width, height);
}

// （Begin・End・ResizeRenderTarget は GameWindow.cpp の中身をそのまま）

// 描画先の切り替え
void GameView::Begin(ID3D11DeviceContext* dc)
{
    if (!dc) return;

    // 現在のビューポートを保存
    UINT numVP = 1;
    dc->RSGetViewports(&numVP, &savedViewport);

    // 自前のビューポートをセット
    D3D11_VIEWPORT vp{};
    vp.Width = static_cast<float>(renderWidth);
    vp.Height = static_cast<float>(renderHeight);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    dc->RSSetViewports(1, &vp);

    // 描画先を自前RTVに切り替え
    dc->OMSetRenderTargets(1, rtv.GetAddressOf(), dsv.Get());

    // クリア
    float clear[4] = { 0.2f, 0.2f, 0.5f, 1.0f };
    dc->ClearRenderTargetView(rtv.Get(), clear);
    dc->ClearDepthStencilView(dsv.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
void GameView::End(ID3D11DeviceContext* dc)
{
    if (!dc) return;

    // ビューポートを元に戻す
    dc->RSSetViewports(1, &savedViewport);

    // 描画先をメインに戻す
    ID3D11RenderTargetView* nullRTV = nullptr;
    dc->OMSetRenderTargets(1, &nullRTV, nullptr);
}


// テクスチャの再生成
void GameView::ResizeRenderTarget(UINT width, UINT height)
{
    if (!device) return;
    if (width == 0 || height == 0) return;

    renderWidth = width;
    renderHeight = height;

    // 古いリソースを解放
    srv.Reset();
    rtv.Reset();
    renderTexture.Reset();
    dsv.Reset();
    depthTexture.Reset();
    outputSrv.Reset();
    outputRtv.Reset();
    outputTexture.Reset();

    HRESULT hr;

    // カラーテクスチャ再生成
    {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = renderWidth;
        desc.Height = renderHeight;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        hr = device->CreateTexture2D(&desc, nullptr, renderTexture.GetAddressOf());
        assert(SUCCEEDED(hr));
        hr = device->CreateRenderTargetView(renderTexture.Get(), nullptr, rtv.GetAddressOf());
        assert(SUCCEEDED(hr));
        hr = device->CreateShaderResourceView(renderTexture.Get(), nullptr, srv.GetAddressOf());
        assert(SUCCEEDED(hr));
    }

    // 深度テクスチャ再生成
    {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = renderWidth;
        desc.Height = renderHeight;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        hr = device->CreateTexture2D(&desc, nullptr, depthTexture.GetAddressOf());
        assert(SUCCEEDED(hr));
        hr = device->CreateDepthStencilView(depthTexture.Get(), nullptr, dsv.GetAddressOf());
        assert(SUCCEEDED(hr));
    }

    // 表示用テクスチャ再生成
    {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = renderWidth;
        desc.Height = renderHeight;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        hr = device->CreateTexture2D(&desc, nullptr, outputTexture.GetAddressOf());
        assert(SUCCEEDED(hr));
        hr = device->CreateRenderTargetView(outputTexture.Get(), nullptr, outputRtv.GetAddressOf());
        assert(SUCCEEDED(hr));
        hr = device->CreateShaderResourceView(outputTexture.Get(), nullptr, outputSrv.GetAddressOf());
        assert(SUCCEEDED(hr));
    }
}

// 表示先の縦横比に合わせて、ゲームカメラのプロジェクションを更新する
void GameView::SetAspect(float aspect)
{
    camera.SetPerspectiveFov(
        DirectX::XMConvertToRadians(camera.GetViewingAngle()),
        aspect,
        0.1f,
        1000.0f
    );
}