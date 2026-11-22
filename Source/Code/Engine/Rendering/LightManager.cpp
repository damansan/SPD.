#include "LightManager.h"
#include "Engine/Resource/GpuResourceUtils.h"
#include "Engine/Utility/misc.h"

// 初期化
void LightManager::Initialize(ID3D11Device* device)
{
    GpuResourceUtils::CreateConstantBuffer(
        device,
        sizeof(CbLights),
        cbLights
    );

    // デフォルトの平行光源を設定
    SetDirectionalLight(
        { 0.5f, -1.0f, 0.5f },
        { 1.0f,  1.0f, 1.0f },
        1.0f
    );
}

// 平行光源の設定
void LightManager::SetDirectionalLight(
    const DirectX::XMFLOAT3& direction,
    const DirectX::XMFLOAT3& color, 
    float intensity)
{
    // 方向を正規化
    DirectX::XMVECTOR dir = DirectX::XMLoadFloat3(&direction);
    dir = DirectX::XMVector3Normalize(dir);
    DirectX::XMStoreFloat3(
        reinterpret_cast<DirectX::XMFLOAT3*>(&directionalLight.direction),
        dir
    );
    directionalLight.direction.w = 0.0f;

    directionalLight.color = {
        color.x, color.y, color.z, intensity
    };
}

// ポイントライトの追加
void LightManager::AddPointLight(
    const DirectX::XMFLOAT3& position, 
    const DirectX::XMFLOAT3& color, 
    float range,
    float intensity)
{
    if ((int)pointLights.size() >= MAX_POINT_LIGHTS) return;

    PointLight light{};
    light.position = { position.x, position.y, position.z, range };
    light.color = { color.x, color.y, color.z, intensity };
    pointLights.push_back(light);
}

// ポイントライトの全削除
void LightManager::ClearPointLights()
{
    pointLights.clear();
}

// 定数バッファをGPUに転送してスロットにセット
void LightManager::Bind(ID3D11DeviceContext* dc)
{
    CbLights cbData{};

    // 平行光源
    cbData.directionalLight = directionalLight;

    // ポイントライト
    int count = (int)pointLights.size();
    if (count > MAX_POINT_LIGHTS) count = MAX_POINT_LIGHTS;
    for (int i = 0; i < count; i++)
        cbData.pointLights[i] = pointLights[i];
    cbData.pointLightCount = count;

    // IBL の強さ
    cbData.iblIntensity = kIblIntensity;

    // GPU転送
    dc->UpdateSubresource(cbLights.Get(), 0, nullptr, &cbData, 0, 0);

    // b2スロットにセット
    dc->PSSetConstantBuffers(2, 1, cbLights.GetAddressOf());
}
