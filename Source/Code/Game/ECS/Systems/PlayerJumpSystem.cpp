#include "PlayerJumpSystem.h"
#include <algorithm>

#include "Game/ECS/Components/PlayerIntentComponent.h"
#include "Game/ECS/Components/PlayerJumpComponent.h"
#include "Game/ECS/Components/PlayerLocomotionComponent.h"

#include "Engine/ECS/Components/StateMachineComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Systems/StateMachineSystem.h"

PlayerJumpSystem::PlayerJumpSystem()
{
	StateMachineSystem::RequireState<PlayerJumpComponent>("Jump", "PlayerJumpSystem");
}

void PlayerJumpSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	(void)elapsedTime;
	(void)context;

	auto view = registry.view<PlayerIntentComponent, PlayerJumpComponent, StateMachineComponent, PlayerLocomotionComponent, MovementComponent>();
	view.each([&](PlayerIntentComponent& intent, PlayerJumpComponent& jump, const StateMachineComponent& stateMachine,
		PlayerLocomotionComponent& loco, MovementComponent& movement)
		{
			if (!stateMachine.IsIn("Jump")) return;

			// 入った瞬間：ジャンプ入力があり、まだ宙に浮いていなければ跳ぶ。そうでなければ落ちるだけ
			if (stateMachine.enteredThisFrame)
			{
				jump.fromGround = intent.jump.buffered && !loco.isAirborne;
				jump.takeoffSpeed = loco.horizontalSpeed;

				if (jump.fromGround)
				{
					// 1回限りの初速は、ステートが直接書いてよい(以降は重力に任せる)
					movement.velocity.y = jump.jumpSpeed;
					intent.jump.Consume();
				}
			}

			// 毎フレーム：空中の指示
			LocomotionCommand& cmd = loco.command;
			cmd.mode = LocomotionMode::Air;
			cmd.moveDir = intent.wantsMove ? intent.moveDir : DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f };
			cmd.targetSpeed = (std::max)(jump.takeoffSpeed, jump.airSpeed); // 踏み切りの勢いを空中で失わないように
			cmd.accelHalfLife = jump.airControlHalfLife;
			cmd.turnSpeed = jump.airTurnSpeed;

			//空中は当たり判定を太くする
			movement.radiusOverride = jump.airRadius;
		});
}