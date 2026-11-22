#include "PlayerWallSenseSystem.h"
#include <cmath>
#include <algorithm>
#include <DirectXMath.h>

#include "Game/ECS/Components/PlayerWallSenseComponent.h"

#include "Engine/ECS/Components/CharacterContactComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"

void PlayerWallSenseSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	(void)elapsedTime;
	(void)context;

	auto view = registry.view<PlayerWallSenseComponent, CharacterContactComponent, MovementComponent>();
	view.each([&](PlayerWallSenseComponent& sense, const CharacterContactComponent& contact, const MovementComponent& movement)
		{
			// フラグリセット
			sense.touchingWall = false;
			sense.approachAngleDeg = 0.0f;
			sense.isWallRight = false;

			// 今のフレームの壁(途中で抜けても、下の猶予の計算は必ず通る)
			do
			{
				if (!contact.hitSide) break;

				// 壁らしさ：ほぼ垂直な面だけ
				const DirectX::XMFLOAT3& n = contact.sideNormal;
				if (fabsf(n.y) > sense.maxWallNormalY) break;

				// 水平成分だけで考える
				const float nLen = sqrtf(n.x * n.x + n.z * n.z);
				if (nLen < 1e-4f) break;
				const float nx = n.x / nLen;
				const float nz = n.z / nLen;

				sense.touchingWall = true;

				const float vLen = sqrtf(movement.velocity.x * movement.velocity.x + movement.velocity.z * movement.velocity.z);
				if (vLen < 1e-4f)
				{
					// 水平に動いていない：向きが決まらないので、正面扱いにしておく
					sense.approachAngleDeg = 90.0f;
					break;
				}
				const float vx = movement.velocity.x / vLen;
				const float vz = movement.velocity.z / vLen;

				// 壁へ向かう成分(1=正面、0=平行、マイナス=離れていく)
				const float into = -(vx * nx + vz * nz);
				sense.approachAngleDeg = DirectX::XMConvertToDegrees(asinf((std::clamp)(into, -1.0f, 1.0f)));

				// 進む向きの右(前が +Z のとき、右は +X)
				const float rightX = vz;
				const float rightZ = -vx;

				// 法線は壁から外向き
				sense.isWallRight = (nx * rightX + nz * rightZ) < 0.0f;
			} while (false);

			// 猶予付き：壁から離れても、少しの間は「壁にいる」とみなす(地面の isAirborne と対)
			sense.wallLostTime = sense.touchingWall ? 0.0f : sense.wallLostTime + elapsedTime;
			sense.nearWall = sense.touchingWall || (sense.wallLostTime < sense.wallGraceTime);
		});
}