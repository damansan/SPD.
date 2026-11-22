#include "GBuffer.h"
#include "Engine/Utility/misc.h"

void GBuffer::Initialize(ID3D11Device* device, UINT width, UINT height)
{
    // 既存リソースを解放してから再生成
    for (UINT i = 0; i < BUFFER_COUNT; i++)
    {
        rtvs[i].Reset();
        srvs[i].Reset();
        rtvPtrs[i] = nullptr;
    }

    // GBufferのフォーマット定義
    DXGI_FORMAT formats[BUFFER_COUNT] =
    {
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, // Albedo（書くとき sRGB へ、読むときリニアへ GPU が変換）
        DXGI_FORMAT_R16G16B16A16_FLOAT,  // Normal
        DXGI_FORMAT_R32G32B32A32_FLOAT,  // Position
        DXGI_FORMAT_R8G8B8A8_UNORM,      // Roughness/Metallic
        DXGI_FORMAT_R16G16B16A16_FLOAT,  // Emissive
        DXGI_FORMAT_R8_UNORM,            // Occlusion
    };

    for (UINT i = 0; i < BUFFER_COUNT; i++)
    {
        // テクスチャ生成
        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = formats[i];
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET
            | D3D11_BIND_SHADER_RESOURCE;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
        HRESULT hr = device->CreateTexture2D(
            &texDesc, nullptr, tex.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        // RTV生成
        hr = device->CreateRenderTargetView(
            tex.Get(), nullptr, rtvs[i].GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        // SRV生成
        hr = device->CreateShaderResourceView(
            tex.Get(), nullptr, srvs[i].GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        // rawポインタ配列にセット
        rtvPtrs[i] = rtvs[i].Get();
    }
}

// Geometryパス開始
void GBuffer::BeginGeometryPass(
    ID3D11DeviceContext* dc,
    ID3D11DepthStencilView* dsv)
{
    // 全GBufferをクリア
    float black[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    for (UINT i = 0; i < BUFFER_COUNT; i++)
        dc->ClearRenderTargetView(rtvs[i].Get(), black);

    // GバッファのRTVをセット（深度は共有）
    dc->OMSetRenderTargets(BUFFER_COUNT, rtvPtrs, dsv);
}

// Geometryパス終了
void GBuffer::EndGeometryPass(ID3D11DeviceContext* dc)
{
    // RTVの束縛を解除
    ID3D11RenderTargetView* nullRTVs[BUFFER_COUNT] = {};
    dc->OMSetRenderTargets(BUFFER_COUNT, nullRTVs, nullptr);
}