#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <string>

class SkyBox
{
public:
    // 初期化
    void Initialize(ID3D11Device* device, const std::string& folderPath);

    // 描画
    void Render(
        ID3D11DeviceContext* dc,
        const DirectX::XMFLOAT4X4& view,
        const DirectX::XMFLOAT4X4& projection,
        const DirectX::XMFLOAT3& cameraPosition
    );

    // SRVゲッター
    ID3D11ShaderResourceView* GetSRV() const { return skyTexture.Get(); }
    ID3D11ShaderResourceView* GetDiffuseIem()  const { return diffuseIem.Get(); }
    ID3D11ShaderResourceView* GetLutGgx()      const { return lutGgx.Get(); }

    // IBL ON/OFF
    bool enabled = true;

private:
    struct CbSkyBox
    {
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
        DirectX::XMFLOAT4   cameraPosition;
        // 16バイトアライメント用パディング
        DirectX::XMFLOAT4   padding;
    };


    // 頂点シェーダー
    Microsoft::WRL::ComPtr<ID3D11VertexShader>       vertexShader;
    // ピクセルシェーダー
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        pixelShader;
    // 入力レイアウト
    Microsoft::WRL::ComPtr<ID3D11InputLayout>        inputLayout;
    // 頂点バッファ
    Microsoft::WRL::ComPtr<ID3D11Buffer>             vertexBuffer;
    // インデックスバッファ
    Microsoft::WRL::ComPtr<ID3D11Buffer>             indexBuffer;
    // キューブマップテクスチャ
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skyTexture;
    // スカイボックス描画用（skybox.ddsがあればこれを使用）
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skyBoxTexture;
    // サンプラーステート
    Microsoft::WRL::ComPtr<ID3D11SamplerState>       samplerState;
    // 深度ステンシルステート（深度書き込みOFF）
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState>  depthStencilState;
    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D11Buffer>             cbSkyBox;

    UINT indexCount = 0;

    // IBL用テクスチャ
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> diffuseIem;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> lutGgx;
};