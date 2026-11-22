#pragma once
#include <d3d11.h>
#include <wrl.h>

class GBuffer
{
public:
    // 初期化
    void Initialize(ID3D11Device* device, UINT width, UINT height);

    // Geometryパス開始
    void BeginGeometryPass(ID3D11DeviceContext* dc,
        ID3D11DepthStencilView* dsv);

    // Geometryパス終了
    void EndGeometryPass(ID3D11DeviceContext* dc);

    // SRVゲッター（LightingパスでGバッファを読む）
    ID3D11ShaderResourceView* GetAlbedoSRV()            const { return srvs[0].Get(); }
    ID3D11ShaderResourceView* GetNormalSRV()            const { return srvs[1].Get(); }
    ID3D11ShaderResourceView* GetPositionSRV()          const { return srvs[2].Get(); }
    ID3D11ShaderResourceView* GetRoughnessMetallicSRV() const { return srvs[3].Get(); }
    ID3D11ShaderResourceView* GetEmissiveSRV()          const { return srvs[4].Get(); }
    ID3D11ShaderResourceView* GetOcclusionSRV()         const { return srvs[5].Get(); }

    // RTV配列ゲッター
    ID3D11RenderTargetView* const* GetRTVs() const { return rtvPtrs; }
    static constexpr UINT BUFFER_COUNT = 6;

private:
    // RTV
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   rtvs[BUFFER_COUNT];
    // SRV
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvs[BUFFER_COUNT];
    // RTVのrawポインタ配列
    ID3D11RenderTargetView* rtvPtrs[BUFFER_COUNT] = {};
};