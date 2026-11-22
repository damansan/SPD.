#pragma once
#include <DirectXMath.h>
#include "Engine/ECS/Reflect.h"

struct TransformComponent
{
	REFLECT_BEGIN(TransformComponent, "Transform")

		REFLECT_FIELD(DirectX::XMFLOAT3, position, (0.0f, 0.0f, 0.0f), ("Position", 0.0f, 0.0f, 0.1f))  // 位置
		REFLECT_FIELD(DirectX::XMFLOAT3, rotation, (0.0f, 0.0f, 0.0f), ("Rotation", 0.0f, 0.0f, 0.01f)) // 回転
		REFLECT_FIELD(DirectX::XMFLOAT3, scale,    (1.0f, 1.0f, 1.0f), ("Scale",    0.0f, 0.0f, 0.01f)) // スケール
		
	REFLECT_END()

		// ワールド行列取得
		DirectX::XMMATRIX GetWorldMatrix() const
	{
		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
		return S * R * T;
	}
};