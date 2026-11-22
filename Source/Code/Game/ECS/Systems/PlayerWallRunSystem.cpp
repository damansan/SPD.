#include "PlayerWallRunSystem.h"
#include <cmath>
#include <algorithm>

#include "Game/ECS/Components/PlayerWallRunComponent.h"
#include "Game/ECS/Components/PlayerWallSenseComponent.h"
#include "Game/ECS/Components/PlayerLocomotionComponent.h"
#include "Game/ECS/Components/PlayerIntentComponent.h"

#include "Game/ECS/Systems/PlayerMovementUtils.h"

#include "Engine/ECS/Components/CharacterContactComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Components/StateMachineComponent.h"
#include "Engine/ECS/Systems/StateMachineSystem.h"

namespace
{
	// 水平の向きを長さ１にする
	bool NormalizeXZ(float& x, float& z)
	{
		const float len = sqrtf(x * x + z * z);
		if (len < 1e-4f) return false;
		x /= len;
		z /= len;
		return true;
	}

	// 向きから「壁へ向かう成分」を取り除き、壁の面に沿った向きにする
	bool ProjectOnWall(float& dx, float& dz, float nx, float nz)
	{
		const float d = dx * nx + dz * nz;
		dx -= d * nx;
		dz -= d * nz;
		return NormalizeXZ(dx, dz);
	}
}

PlayerWallRunSystem::PlayerWallRunSystem()
{
	StateMachineSystem::RequireState<PlayerWallRunComponent>("WallRun", "PlayerWallRunSystem");
}

void PlayerWallRunSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	(void)context;

	auto view = registry.view<PlayerWallRunComponent, PlayerWallSenseComponent, CharacterContactComponent,
		StateMachineComponent, PlayerLocomotionComponent, MovementComponent, TransformComponent, PlayerIntentComponent>();

	view.each([&](PlayerWallRunComponent& wallRun, const PlayerWallSenseComponent& sense, const CharacterContactComponent& contact,
		const StateMachineComponent& stateMachine, PlayerLocomotionComponent& loco, MovementComponent& movement, const TransformComponent& transform,
		PlayerIntentComponent& intent)
		{
			// 現在がWallRunでなければ何もしない
			if (!stateMachine.IsIn("WallRun")) return;

			// 壁に当たっていれば、法線を取り直す
			float nx = contact.sideNormal.x;
			float nz = contact.sideNormal.z;
			if (contact.hitSide && NormalizeXZ(nx, nz))
			{
				wallRun.wallNormal = { nx, 0.0f, nz };
			}
			const float wnx = wallRun.wallNormal.x;
			const float wnz = wallRun.wallNormal.z;

			// WallRun 開始
			if (stateMachine.enteredThisFrame)
			{
				// 走る向き：現在の速度を壁の面に沿わせる
				float dx = movement.velocity.x;
				float dz = movement.velocity.z;

				if (!ProjectOnWall(dx, dz, wnx, wnz))
				{
					// 速度から決まらない場合は体の向きを使う
					const DirectX::XMFLOAT3 forward = PlayerMovementUtils::YawToForward(transform.rotation.y);
					dx = forward.x;
					dz = forward.z;
					ProjectOnWall(dx, dz, wnx, wnz);
				}
				wallRun.runDir = { dx, 0.0f, dz };

				// 走る速さ：入ってきた勢いを保つ
				const float planarSpeed = sqrtf(movement.velocity.x * movement.velocity.x + movement.velocity.z * movement.velocity.z);
				wallRun.runSpeed = (std::max)(planarSpeed, wallRun.minRunSpeed);

				// 壁が左右どちらにあるかを決める
				wallRun.isWallRight = sense.isWallRight;
				
				// 状態を初期化
				wallRun.elapsed = 0.0f;
				wallRun.kicked = false;
				wallRun.kickTime = 0.0f;
				wallRun.finished = false;

				// 前回の壁走りから着地を挟んでいるか
				const bool chained = (loco.landingCount == wallRun.lastLandingCount);
				wallRun.lastLandingCount = loco.landingCount;

				// 始まる区間と、縦の速さを決める
				wallRun.isClimbing = false;
				wallRun.isFalling = false;
				if (!chained)
				{
					// 最初の壁走り：駆け上がらせる
					wallRun.isClimbing = true;
					wallRun.climbStartY = transform.position.y;
					movement.velocity.y = wallRun.climbSpeed;
				}
				else
				{
					// 連続の壁走り：駆け下がらせる
					wallRun.isFalling = true;
					movement.velocity.y = (std::clamp)(movement.velocity.y, wallRun.minEnterVerticalSpeed, wallRun.maxEnterVerticalSpeed);
				}
			}
			else
			{
				// 走っている向きを、今の壁の面に沿わせ直す
				float dx = wallRun.runDir.x;
				float dz = wallRun.runDir.z;
				if (ProjectOnWall(dx, dz, wnx, wnz))
				{
					wallRun.runDir = { dx, 0.0f, dz };
				}
			}

			wallRun.elapsed += elapsedTime;

			const float gravity = fabsf(movement.gravity);


			// 駆け上がりの終わりを判断する
			if (wallRun.isClimbing)
			{
				float arcHeight = 0.0f;
				const float riseGravity = gravity * wallRun.riseGravityScale;
				if (riseGravity > 0.0f)
				{
					arcHeight = wallRun.climbSpeed * wallRun.climbSpeed / (2.0f * riseGravity);
				}
				float straightHeight = wallRun.climbHeight - arcHeight;
				if (straightHeight < 0.0f)
				{
					straightHeight = 0.0f;
				}

				if (transform.position.y - wallRun.climbStartY >= straightHeight)
				{
					wallRun.isClimbing = false;
				}
			}

			// 駆け下がり中は、推進力を少しずつ下げる
			if (!wallRun.isClimbing && !wallRun.isFalling && movement.velocity.y <= 0.0f)
			{
				wallRun.isFalling = true;
			}

			// 推進力を下げ始める
			if (wallRun.isFalling)
			{
				wallRun.runSpeed *= powf(0.5f, elapsedTime / wallRun.fallSpeedHalfLife);
			}



			// 壁を蹴る
			if (!wallRun.kicked && intent.jump.buffered)
			{
				const float forward = wallRun.runSpeed * wallRun.kickForwardScale;
				movement.velocity.x = wallRun.runDir.x * forward + wnx * wallRun.kickOutSpeed;
				movement.velocity.z = wallRun.runDir.z * forward + wnz * wallRun.kickOutSpeed;
				movement.velocity.y = wallRun.kickUpSpeed;

				intent.jump.Consume();
				wallRun.kicked = true;
			}
			movement.radiusOverride = wallRun.radius;

			// 蹴った後：ロックの時間を数える
			if (wallRun.kicked)
			{
				wallRun.kickTime += elapsedTime;
			}

			// 壁蹴り後、ロック時間が過ぎた
			const bool kickDone = wallRun.kicked && wallRun.kickTime >= wallRun.kickLockTime;
			// 壁が無くなった
			const bool wallLost = !wallRun.kicked && !sense.nearWall;
			// 走る速さが一定以下になった
			const bool speedLost = !wallRun.kicked && wallRun.runSpeed < wallRun.enterMinSpeed; 
			// 上記のどれか or 地面についたら WallRun を終了する
			wallRun.finished = kickDone || wallLost || speedLost || movement.isGrounded;

			// 蹴った直後は Locomotion に触らない
			if (wallRun.kicked)
			{
				loco.command.mode = LocomotionMode::External;
				return;
			}

			// 走る指示を Locomotion に指示する
			const float stick = sense.touchingWall ? wallRun.stickSpeed : 0.0f;
			float mx = wallRun.runDir.x * wallRun.runSpeed - wnx * stick;
			float mz = wallRun.runDir.z * wallRun.runSpeed - wnz * stick;
			const float targetSpeed = sqrtf(mx * mx + mz * mz);
			NormalizeXZ(mx, mz);

			LocomotionCommand& cmd = loco.command;
			cmd.mode = LocomotionMode::Directional;
			cmd.moveDir = { mx, 0.0f, mz };
			cmd.targetSpeed = targetSpeed;
			cmd.accelHalfLife = wallRun.accelHalfLife;
			cmd.faceMoveDir = true;

			// 重力を Movement へ指示する
			if (wallRun.isClimbing)
			{
				// 駆け上がり：重力 0
				movement.gravityScale = 0.0f; 
			}
			else if (wallRun.isFalling)
			{
				// 駆け下がり：失速しながら下がる
				movement.gravityScale = wallRun.fallGravityScale;
			}
			else
			{
				// 頂上付近：上昇が弱まる
				movement.gravityScale = wallRun.riseGravityScale;
			}
		});
}