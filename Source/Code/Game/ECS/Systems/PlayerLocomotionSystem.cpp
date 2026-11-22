#include "PlayerLocomotionSystem.h"
#include <cmath>
#include <algorithm>
#include <DirectXMath.h>

#include "Game/ECS/Components/PlayerLocomotionComponent.h"
#include "Game/ECS/Systems/PlayerMovementUtils.h"
#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"

namespace
{
	// 指数的な減衰は0に届かないので、これより遅くなったら0にする
	constexpr float kStopSpeed = 0.05f;

	float PlanarLength(float x, float z) { return sqrtf(x * x + z * z); }

	bool HasDirection(const DirectX::XMFLOAT3& dir) { return PlanarLength(dir.x, dir.z) > 0.0001f; }

	// 指示された方向へ体を回し、回った後の向きのずれ(rad)を返す
	float TurnTowards(const LocomotionCommand& cmd, TransformComponent& transform, float elapsedTime)
	{
		if (!HasDirection(cmd.moveDir)) return 0.0f;

		float targetYaw = PlayerMovementUtils::CalcFacingYaw(cmd.moveDir.x, cmd.moveDir.z);
		transform.rotation.y = PlayerMovementUtils::RotateTowards(transform.rotation.y, targetYaw, cmd.turnSpeed, elapsedTime);
		return PlayerMovementUtils::AngleDiff(transform.rotation.y, targetYaw);
	}

	// Ground に入る時：実際の速度を「体の向きに沿った速さ」と「持ち越しの速度」に分けて取り込む
	void AbsorbIntoGround(PlayerLocomotionComponent& loco, const MovementComponent& movement, const TransformComponent& transform)
	{
		DirectX::XMFLOAT3 forward = PlayerMovementUtils::YawToForward(transform.rotation.y);

		// 体の向きに沿った成分(後ろ向きは0とする)
		float along = movement.velocity.x * forward.x + movement.velocity.z * forward.z;
		loco.currentSpeed = (std::max)(0.0f, along);

		// それ以外(横・後ろ向き)は、持ち越しの速度として残す
		loco.carryVelocity.x = movement.velocity.x - forward.x * loco.currentSpeed;
		loco.carryVelocity.z = movement.velocity.z - forward.z * loco.currentSpeed;
	}

	// 地上：体の向きに沿って進む
	void DriveGround(PlayerLocomotionComponent& loco, const LocomotionCommand& cmd,
		MovementComponent& movement, TransformComponent& transform, float elapsedTime)
	{
		// 向き
		float facingError = TurnTowards(cmd, transform, elapsedTime);

		// 自分で走っている速さ
		float alignment = (std::max)(0.0f, cosf(facingError));
		float desiredSpeed = cmd.targetSpeed * alignment;

		float halfLife = (desiredSpeed > loco.currentSpeed) ? cmd.accelHalfLife : cmd.decelHalfLife;
		loco.currentSpeed = PlayerMovementUtils::Damper(loco.currentSpeed, desiredSpeed, halfLife, elapsedTime);
		if (desiredSpeed <= 0.0f && loco.currentSpeed < kStopSpeed)
		{
			loco.currentSpeed = 0.0f;
		}

		// 持ち越しの速度
		loco.carryVelocity.x = PlayerMovementUtils::Damper(loco.carryVelocity.x, 0.0f, cmd.carryHalfLife, elapsedTime);
		loco.carryVelocity.z = PlayerMovementUtils::Damper(loco.carryVelocity.z, 0.0f, cmd.carryHalfLife, elapsedTime);
		if (PlanarLength(loco.carryVelocity.x, loco.carryVelocity.z) < kStopSpeed)
		{
			loco.carryVelocity = { 0.0f, 0.0f, 0.0f };
		}

		// 速度 = 体の向き × 速さ ＋ 持ち越し
		DirectX::XMFLOAT3 forward = PlayerMovementUtils::YawToForward(transform.rotation.y);
		movement.velocity.x = forward.x * loco.currentSpeed + loco.carryVelocity.x;
		movement.velocity.z = forward.z * loco.currentSpeed + loco.carryVelocity.z;

		// 確認用
		loco.desiredSpeed = desiredSpeed;
		loco.facingErrorDeg = DirectX::XMConvertToDegrees(facingError);
	}

	// 空中：速度のベクトルを保つ
	void DriveAir(PlayerLocomotionComponent& loco, const LocomotionCommand& cmd,
		MovementComponent& movement, TransformComponent& transform, float elapsedTime)
	{
		// 向き
		float facingError = TurnTowards(cmd, transform, elapsedTime);

		// 速度：入力が無ければ勢いをそのまま保つ
		if (HasDirection(cmd.moveDir))
		{
			float targetX = cmd.moveDir.x * cmd.targetSpeed;
			float targetZ = cmd.moveDir.z * cmd.targetSpeed;
			movement.velocity.x = PlayerMovementUtils::Damper(movement.velocity.x, targetX, cmd.accelHalfLife, elapsedTime);
			movement.velocity.z = PlayerMovementUtils::Damper(movement.velocity.z, targetZ, cmd.accelHalfLife, elapsedTime);
		}

		// 着地した後でも参照できるように
		loco.landingHorizontalSpeed = PlanarLength(movement.velocity.x, movement.velocity.z);
		loco.landingVerticalSpeed = movement.velocity.y;

		// 確認用
		loco.currentSpeed = loco.landingHorizontalSpeed;
		loco.desiredSpeed = cmd.targetSpeed;
		loco.facingErrorDeg = DirectX::XMConvertToDegrees(facingError);
	}

	// 向きとは別の方向に進む：体の向きは変えず、速度だけを指示された方向へ寄せる
	void DriveDirectional(PlayerLocomotionComponent& loco, const LocomotionCommand& cmd,
		MovementComponent& movement, float elapsedTime)
	{
		// 速度：指示された方向 × 速さ へダンパーで近づける
		float targetX = cmd.moveDir.x * cmd.targetSpeed;
		float targetZ = cmd.moveDir.z * cmd.targetSpeed;
		movement.velocity.x = PlayerMovementUtils::Damper(movement.velocity.x, targetX, cmd.accelHalfLife, elapsedTime);
		movement.velocity.z = PlayerMovementUtils::Damper(movement.velocity.z, targetZ, cmd.accelHalfLife, elapsedTime);

		// 確認用
		loco.currentSpeed = PlanarLength(movement.velocity.x, movement.velocity.z);
		loco.desiredSpeed = cmd.targetSpeed;
		loco.facingErrorDeg = 0.0f;
	}
}

void PlayerLocomotionSystem::Update(entt::registry& registry, float elapsedTime)
{
	auto view = registry.view<PlayerLocomotionComponent, MovementComponent, TransformComponent>();
	view.each([&](PlayerLocomotionComponent& loco, MovementComponent& movement, TransformComponent& transform)
		{
			const LocomotionCommand& cmd = loco.command;

			// 体の状態：着地した回数
			if (movement.isGrounded && loco.airTime > 0.0f)
			{
				loco.landingCount++;
			}
			// 体の状態：接地していない時間
			loco.airTime = movement.isGrounded ? 0.0f : loco.airTime + elapsedTime;
			loco.isAirborne = (loco.airTime >= loco.fallGraceTime);

			// 指示があれば、向きを進む方向へ一気に合わせる
			if (HasDirection(cmd.faceDir))
			{
				transform.rotation.y = PlayerMovementUtils::CalcFacingYaw(cmd.faceDir.x, cmd.faceDir.z);
			}
			else if (cmd.faceMoveDir && HasDirection(cmd.moveDir))
			{
				transform.rotation.y = PlayerMovementUtils::CalcFacingYaw(cmd.moveDir.x, cmd.moveDir.z);
			}

			// 他のモードから Ground に切り替わった時だけ、実際の速度を取り込む
			if (cmd.mode == LocomotionMode::Ground && loco.lastMode != LocomotionMode::Ground) 
			{
				AbsorbIntoGround(loco, movement, transform);
			}

			switch (cmd.mode)
			{
			case LocomotionMode::Ground:
				DriveGround(loco, cmd, movement, transform, elapsedTime);
				break;

			case LocomotionMode::Air:
				DriveAir(loco, cmd, movement, transform, elapsedTime);
				break;

			case LocomotionMode::Directional:
				DriveDirectional(loco, cmd, movement, elapsedTime);
				break;

			case LocomotionMode::External:
				// velocity.xz は外部が書く
				loco.desiredSpeed = 0.0f;
				loco.facingErrorDeg = 0.0f;
				break;
			}

			// 体の状態：今の勢い
			loco.horizontalSpeed = PlanarLength(movement.velocity.x, movement.velocity.z);
			loco.verticalSpeed = movement.velocity.y;

			// 指示は1フレーム限り
			loco.lastMode = cmd.mode;
			loco.command = LocomotionCommand{};
		});
}