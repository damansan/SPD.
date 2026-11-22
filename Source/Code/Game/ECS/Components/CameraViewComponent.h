#pragma once
#include <DirectXMath.h>
#include "Engine/ECS/Reflect.h"

struct CameraViewComponent
{
	REFLECT_BEGIN(CameraViewComponent, "Camera View")

		REFLECT_FIELD(DirectX::XMFLOAT3,   eye,  (0.0f, 0.0f, 0.0f), ("Eye",    0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 最終の eye
		REFLECT_FIELD(DirectX::XMFLOAT3, target, (0.0f, 0.0f, 0.0f), ("Target", 0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 最終の target
		
		REFLECT_FIELD(float, fov,  (0.0f), ("Fov",  0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 最終の FOV
		REFLECT_FIELD(float, roll, (0.0f), ("Roll", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 最終の傾き
		
		REFLECT_FIELD(bool, valid, (false), ("Valid", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 値ができているか

	REFLECT_END()
};