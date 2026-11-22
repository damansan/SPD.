#pragma once
#include <DirectXMath.h>

// カメラコンポーネントの共通データ
struct BaseCameraComponent
{
    DirectX::XMFLOAT3 currentEye = { 0.0f, 5.0f, 10.0f };
    DirectX::XMFLOAT3 currentTarget = { 0.0f, 0.0f,  0.0f };
};
