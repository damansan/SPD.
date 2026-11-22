#include "PlayerDodgeSystem.h"
#include <cmath>

#include "Game/ECS/Components/PlayerIntentComponent.h"
#include "Game/ECS/Components/PlayerDodgeComponent.h"
#include "Game/ECS/Components/PlayerLocomotionComponent.h"
#include "Game/ECS/Systems/PlayerMovementUtils.h"

#include "Engine/ECS/Components/StateMachineComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Systems/StateMachineSystem.h"

namespace
{
	// 体の向きから見た角度差から、4方向のどれかを決める
	PlayerDodgeDirection ClassifyDirection(float diff)
	{
		const float quarter = DirectX::XM_PIDIV4; // 45度

		if (diff > -quarter && diff <= quarter)         return PlayerDodgeDirection::Front;
		if (diff > quarter && diff <= quarter * 3.0f)   return PlayerDodgeDirection::Right;
		if (diff > -quarter * 3.0f && diff <= -quarter) return PlayerDodgeDirection::Left;
		return PlayerDodgeDirection::Back;
	}
}

PlayerDodgeSystem::PlayerDodgeSystem()
{
	StateMachineSystem::RequireState<PlayerDodgeComponent>("Dodge", "PlayerDodgeSystem");
}

void PlayerDodgeSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	(void)context;

	auto view = registry.view<PlayerIntentComponent, PlayerDodgeComponent, StateMachineComponent, PlayerLocomotionComponent, TransformComponent>();

	view.each([&](PlayerIntentComponent& intent, PlayerDodgeComponent& dodge, const StateMachineComponent& stateMachine,
		PlayerLocomotionComponent& loco, const TransformComponent& transform)
		{

			if (!stateMachine.IsIn("Dodge")) return;

			// 入った瞬間 回避方向を決める
			if (stateMachine.enteredThisFrame)
			{
				const float bodyYaw = transform.rotation.y;

				// スティック入力が無ければ、体の後ろ
				float dodgeYaw = bodyYaw + DirectX::XM_PI;
				dodge.faceDodgeDir = false;

				if (intent.wantsMove)
				{
					float inputYaw = PlayerMovementUtils::CalcFacingYaw(intent.moveDir.x, intent.moveDir.z);
					float diff = PlayerMovementUtils::AngleDiff(bodyYaw, inputYaw);

					// 体から見た8方向(-4?4。0 が前、+ が右回り)
					int sector = static_cast<int>(std::round(diff / DirectX::XM_PIDIV4));

					if (sector >= -1 && sector <= 1)
					{
						// 前左・前・前右：45度刻みに丸めた方向を向いて、前回避
						dodgeYaw = bodyYaw + DirectX::XM_PIDIV4 * static_cast<float>(sector);
						dodge.faceDodgeDir = true;
					}
					else
					{
						// 横・後ろ：今までどおり、スティックの方向へ(右後ろ・左後ろは保留)
						dodgeYaw = inputYaw;
					}
				}
				dodge.dodgeDir = PlayerMovementUtils::YawToForward(dodgeYaw);

				// クリップの選択用：入った瞬間だけ決め、回避中は変えない
				dodge.direction = dodge.faceDodgeDir
					? PlayerDodgeDirection::Front
					: ClassifyDirection(PlayerMovementUtils::AngleDiff(bodyYaw, dodgeYaw));
				dodge.isFront = (dodge.direction == PlayerDodgeDirection::Front);
				dodge.isBack = (dodge.direction == PlayerDodgeDirection::Back);
				dodge.isLeft = (dodge.direction == PlayerDodgeDirection::Left);
				dodge.isRight = (dodge.direction == PlayerDodgeDirection::Right);

				dodge.elapsed = 0.0f;
				dodge.finished = false;

				// 入ったステートが入力を消費する
				intent.dodge.Consume();
			}
			// 回避中 スティック入力があれば、回避方向を入力のほうへ曲げる
			else if (intent.wantsMove)
			{
				float dodgeYaw = PlayerMovementUtils::CalcFacingYaw(dodge.dodgeDir.x, dodge.dodgeDir.z);
				float inputYaw = PlayerMovementUtils::CalcFacingYaw(intent.moveDir.x, intent.moveDir.z);
				dodgeYaw = PlayerMovementUtils::RotateTowards(dodgeYaw, inputYaw, dodge.turnSpeed, elapsedTime);
				dodge.dodgeDir = PlayerMovementUtils::YawToForward(dodgeYaw);
			}

			// 毎フレーム 時間を進め、体から見た回避方向を求める
			dodge.elapsed += elapsedTime;

			float localDiff = PlayerMovementUtils::AngleDiff(
				transform.rotation.y,
				PlayerMovementUtils::CalcFacingYaw(dodge.dodgeDir.x, dodge.dodgeDir.z));
			dodge.localX = sinf(localDiff);
			dodge.localZ = cosf(localDiff);

			// 終わりの判定：最低時間を過ぎてR2を離した、または最大時間に達した
			if (!dodge.finished)
			{
				bool released = !intent.dodge.held && dodge.elapsed >= dodge.minTime;
				bool timeUp = dodge.elapsed >= dodge.maxTime;
				dodge.finished = released || timeUp;
			}

			// 毎フレーム：Locomotion への指示
			LocomotionCommand& cmd = loco.command;
			cmd.mode = LocomotionMode::Directional;
			cmd.moveDir = dodge.dodgeDir;
			cmd.targetSpeed = dodge.dodgeSpeed;
			cmd.accelHalfLife = dodge.accelHalfLife;

			// 前方3方向：回避中ずっと、体を回避方向へ向ける
			// それ以外：走り出す時だけ、抜ける前に向きを回避方向へ合わせておく
			cmd.faceMoveDir = dodge.faceDodgeDir || (dodge.finished && intent.wantsMove);
		});
}