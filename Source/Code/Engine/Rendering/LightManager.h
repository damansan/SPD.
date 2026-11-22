#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <vector>

#include "Engine/Rendering/ShaderLimits.h"

class LightManager
{
public:
    // ライト構造体
    struct DirectionalLight
    {
        DirectX::XMFLOAT4 direction; // xyz=方向 w=未使用
        DirectX::XMFLOAT4 color;     // xyz=色 w=強度
    };

    struct PointLight
    {
        DirectX::XMFLOAT4 position;  // xyz=位置 w=範囲
        DirectX::XMFLOAT4 color;     // xyz=色 w=強度
    };

    // 定数バッファ構造体
    struct CbLights
    {
        DirectionalLight directionalLight;
        PointLight       pointLights[MAX_POINT_LIGHTS];
        int              pointLightCount;
        float            iblIntensity;  // IBLの強さ
        DirectX::XMFLOAT2 padding;      // 16バイトアラインメント用
    };

public:
    // 初期化
    void Initialize(ID3D11Device* device);

    // 平行光源の設定
    void SetDirectionalLight(
        const DirectX::XMFLOAT3& direction,
        const DirectX::XMFLOAT3& color,
        float intensity);

    // ポイントライトの追加
    void AddPointLight(
        const DirectX::XMFLOAT3& position,
        const DirectX::XMFLOAT3& color,
        float range,
        float intensity);

    // ポイントライトの全削除
    void ClearPointLights();

    // 定数バッファをGPUに転送してスロットにセット
    void Bind(ID3D11DeviceContext* dc);

    // ゲッター
    const DirectionalLight& GetDirectionalLight() const { return directionalLight; }

private:
    static constexpr float kIblIntensity = 1.0f;

    DirectionalLight directionalLight{};
    std::vector<PointLight> pointLights;

    Microsoft::WRL::ComPtr<ID3D11Buffer> cbLights;
};