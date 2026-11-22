#include "CameraViewSystem.h"
#include <cmath>

#include "Game/ECS/Components/CameraViewComponent.h"
#include "Game/ECS/Components/CameraEffectComponent.h"
#include "Game/ECS/Components/GameCameraComponent.h"

void CameraViewSystem::Update(entt::registry& registry)
{
	auto view = registry.view<CameraViewComponent, GameCameraComponent>();
	view.each([&](entt::entity entity, CameraViewComponent& camView, const GameCameraComponent& cam)
		{
			// 土台の位置合わせがまだなら、作らない
			camView.valid = false;
			if (!cam.initialized) return;

			// 土台
			camView.eye = cam.base.currentEye;
			camView.target = cam.base.currentTarget;
			camView.fov = cam.fov;
			camView.roll = 0.0f;

			// ＋ 演出
			const auto* effect = registry.try_get<CameraEffectComponent>(entity);
			if (effect)
			{
				camView.eye.x += effect->eyeOffset.x;
				camView.eye.y += effect->eyeOffset.y;
				camView.eye.z += effect->eyeOffset.z;
				camView.target.x += effect->targetOffset.x;
				camView.target.y += effect->targetOffset.y;
				camView.target.z += effect->targetOffset.z;
				camView.fov += effect->fovOffset;
				camView.roll += effect->roll;
			}

			// あたり判定

			camView.valid = true;
		});
}

// Camera に渡す
void CameraViewSystem::ApplyToCamera(entt::registry& registry, Camera& camera)
{
	auto view = registry.view<CameraViewComponent>();
	for (auto entity : view)
	{
		const auto& camView = view.get<CameraViewComponent>(entity);
		if (!camView.valid) continue;

		// 傾き：見る向きを軸に、上の向き(0,1,0)を roll だけ回す
		const DirectX::XMVECTOR eye = DirectX::XMLoadFloat3(&camView.eye);
		const DirectX::XMVECTOR target = DirectX::XMLoadFloat3(&camView.target);
		const DirectX::XMVECTOR forward = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(target, eye));
		const DirectX::XMMATRIX rollMatrix = DirectX::XMMatrixRotationAxis(forward, DirectX::XMConvertToRadians(camView.roll));
		DirectX::XMFLOAT3 up;
		DirectX::XMStoreFloat3(&up, DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rollMatrix));

		camera.SetLookAt(camView.eye, camView.target, up);
		camera.SetFovDeg(camView.fov);
		break; // カメラは1つだけ
	}
}
