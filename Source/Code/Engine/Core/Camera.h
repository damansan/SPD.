#pragma once
#include <DirectXMath.h>

class Camera
{
public:
    Camera() = default;
    ~Camera() = default;

    // 初期化
    void Initialize(float screenWidth, float screenHeight);

    // 注視点設定
    void SetLookAt(
        const DirectX::XMFLOAT3& eye,
        const DirectX::XMFLOAT3& target,
        const DirectX::XMFLOAT3& up
    );

    // パースペクティブ設定
    void SetPerspectiveFov(
        float fovY, float aspect,
        float nearZ, float farZ
    );

    // Fovのセット
    void SetFovDeg(float fovDeg);

    // ゲッター
    const DirectX::XMFLOAT4X4& GetView()        const { return view; }
    const DirectX::XMFLOAT4X4& GetProjection()  const { return projection; }
    const DirectX::XMFLOAT3& GetEye()           const { return eye; }
    const DirectX::XMFLOAT3& GetTarget()        const { return target; }
    const DirectX::XMFLOAT3& GetFront()         const { return front; }
    const DirectX::XMFLOAT3& GetRight()         const { return right; }
    const DirectX::XMFLOAT3& GetUp()            const { return up; }
    float                    GetViewingAngle()  const { return viewingAngle; }
    void                     SetViewingAngle(float v) { viewingAngle = v; }

private:
    DirectX::XMFLOAT4X4 view = {};
    DirectX::XMFLOAT4X4 projection = {};
    DirectX::XMFLOAT3   eye = {};
    DirectX::XMFLOAT3   target = {};
    DirectX::XMFLOAT3   up = {};
    DirectX::XMFLOAT3   front = {};
    DirectX::XMFLOAT3   right = {};
    float               viewingAngle = 40.0f;

    // 最後に SetPerspectiveFov で使った値
    float               aspect = 16.0f / 9.0f;
    float               nearZ = 0.1f;
    float               farZ = 1000.0f;
};