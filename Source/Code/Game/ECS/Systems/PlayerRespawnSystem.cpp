#include "PlayerRespawnSystem.h"

#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Components/PhysicsComponents.h"

void PlayerRespawnSystem::Update(entt::registry& registry, float elapsedTime)
{
	auto view = registry.view<PlayerRespawnComponent, TransformComponent, CharacterControllerComponent>();
	for (auto entity : view)
	{
		auto& respawn = view.get<PlayerRespawnComponent>(entity);
		auto& transform = view.get<TransformComponent>(entity);
		auto& ctrl = view.get<CharacterControllerComponent>(entity);
		if (!ctrl.controller) continue; // Controller ができるまで待つ

		// 初回：今の位置を戻る場所として覚える
		if (!respawn.hasSpawn)
		{
			respawn.spawnPosition = transform.position;
			respawn.hasSpawn = true;
		}

		// 落ちたらフェードアウト開始
		if (!respawn.respawning && transform.position.y < respawn.killY)
		{
			respawn.respawning = true;
		}

		if (respawn.respawning)
		{
			// 黒くしていく
			respawn.fade += elapsedTime / respawn.fadeOutTime;
			if (respawn.fade < 1.0f) continue;

			// 真っ黒になった：開始位置へ置き直す
			respawn.fade = 1.0f;
			respawn.respawning = false;
			respawn.blackTimer = respawn.blackTime;

			const DirectX::XMFLOAT3& p = respawn.spawnPosition;
			ctrl.controller->setFootPosition(physx::PxExtendedVec3(p.x, p.y, p.z));
			transform.position = p;
			transform.rotation = { 0.0f, 0.0f, 0.0f };
			// 落下の勢いを消す
			if (auto* movement = registry.try_get<MovementComponent>(entity))
			{
				movement->velocity = { 0.0f, 0.0f, 0.0f };
			}
		}
		else if (respawn.blackTimer > 0.0f)
		{
			// 真っ暗のまま待つ
			respawn.blackTimer -= elapsedTime;
		}
		else
		{
			// 戻していく
			respawn.fade -= elapsedTime / respawn.fadeInTime;
			if (respawn.fade < 0.0f)
			{
				respawn.fade = 0.0f;
			}
		}
	}
}