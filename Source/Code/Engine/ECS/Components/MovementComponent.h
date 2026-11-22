#pragma once
#include <DirectXMath.h>
#include "Engine/ECS/Reflect.h"

struct MovementComponent
{
	REFLECT_BEGIN(MovementComponent, "Movement")

		// 設定値
		REFLECT_FIELD(float,             gravity,     (-20.0f),           ("Gravity",   -100.0f, 0.0f         ))  
		REFLECT_FIELD(DirectX::XMFLOAT3, velocity,    (0.0f, 0.0f, 0.0f), ("Velocity",   0.0f,   0.0f,  0.01f ))
		REFLECT_FIELD(bool,              isGrounded,  (false),            ("Is Grounded"                      ))
		REFLECT_FIELD(float,             planarSpeed, (0.0f),             ("Planar Speed", 0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient))

		// 重力の倍率
		REFLECT_FIELD(float, gravityScale, (1.0f), ("Gravity Scale", 0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient))
		// 当たり判定(高さ)の倍率
		REFLECT_FIELD(float, heightScale, (1.0f), ("Height Scale", 0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient))
		// 当たり判定 (幅) の倍率
		REFLECT_FIELD(float, radiusOverride, (0.0f), ("Radius Override", 0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient))
	REFLECT_END()
};