#include "PlayerMoveSystem.h"
#include "Game/ECS/Components/PlayerIntentComponent.h"
#include "Game/ECS/Components/PlayerMoveComponent.h"
#include "Game/ECS/Components/PlayerLocomotionComponent.h"

#include "Engine/ECS/Components/StateMachineComponent.h"
#include "Engine/ECS/Systems/StateMachineSystem.h"

PlayerMoveSystem::PlayerMoveSystem()
{
	StateMachineSystem::RequireState<PlayerMoveComponent>("Move", "PlayerMoveSystem");
}

void PlayerMoveSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	(void)context;

	auto view = registry.view<PlayerIntentComponent, PlayerMoveComponent, StateMachineComponent, PlayerLocomotionComponent>();
	view.each([&](const PlayerIntentComponent& intent, PlayerMoveComponent& move,
		const StateMachineComponent& stateMachine, PlayerLocomotionComponent& loco)
		{
			if (!stateMachine.IsIn("Move")) return;

			// 入った瞬間に計測をやり直す
			if (stateMachine.enteredThisFrame)
			{
				move.runTime = 0.0f;
			}

			// Run の速さに入っている間だけ時間を進める
			const bool atRunSpeed = loco.currentSpeed >= move.runSpeed * move.runReachRatio;
			move.runTime = atRunSpeed ? move.runTime + elapsedTime : 0.0f;

			// 意図を、Locomotion への指示に変える
			LocomotionCommand& cmd = loco.command;
			cmd.mode = LocomotionMode::Ground;
			cmd.moveDir = intent.moveDir;
			cmd.targetSpeed = intent.wantsRun ? move.runSpeed : move.walkSpeed;
			cmd.accelHalfLife = move.accelHalfLife;
			cmd.decelHalfLife = move.decelHalfLife;
			cmd.turnSpeed = move.turnSpeed;
		});
}