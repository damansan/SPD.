#include "Camera.h"

// 初期化
void Camera::Initialize(float screenWidth, float screenHeight)
{
    SetLookAt(
        DirectX::XMFLOAT3(0, 5, 10),
        DirectX::XMFLOAT3(0, 0, 0),
        DirectX::XMFLOAT3(0, 1, 0)
    );
    SetPerspectiveFov(
        DirectX::XMConvertToRadians(viewingAngle),
        screenWidth / screenHeight,
        0.1f,
        1000.0f
    );
}

// 注視点設定
void Camera::SetLookAt(
    const DirectX::XMFLOAT3& eye,
    const DirectX::XMFLOAT3& target,
    const DirectX::XMFLOAT3& up)
{
    DirectX::XMVECTOR Eye = DirectX::XMLoadFloat3(&eye);
    DirectX::XMVECTOR Target = DirectX::XMLoadFloat3(&target);
    DirectX::XMVECTOR Up = DirectX::XMLoadFloat3(&up);
    DirectX::XMMATRIX View = DirectX::XMMatrixLookAtLH(Eye, Target, Up);
    DirectX::XMStoreFloat4x4(&this->view, View);

    // 方向ベクトル取得
    this->right.x = view._11;
    this->right.y = view._21;
    this->right.z = view._31;

    this->up.x = view._12;
    this->up.y = view._22;
    this->up.z = view._32;

    this->front.x = view._13;
    this->front.y = view._23;
    this->front.z = view._33;

    this->eye = eye;
    this->target = target;
}

// パースペクティブ設定
void Camera::SetPerspectiveFov(
    float fovY, float aspect,
    float nearZ, float farZ)
{
    // FOV だけを変える時のために覚えておく
    this->aspect = aspect;
    this->nearZ = nearZ;
    this->farZ = farZ;

    DirectX::XMMATRIX Projection =
        DirectX::XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ);
    DirectX::XMStoreFloat4x4(&projection, Projection);
}

// Fovのセット
void Camera::SetFovDeg(float fovDeg)
{
    viewingAngle = fovDeg;
    SetPerspectiveFov(DirectX::XMConvertToRadians(fovDeg), aspect, nearZ, farZ);
}
