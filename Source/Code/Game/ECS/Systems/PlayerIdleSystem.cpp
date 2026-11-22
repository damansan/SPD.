#include "PlayerIdleSystem.h"
#include "Game/ECS/Components/PlayerIdleComponent.h"
#include "Game/ECS/Components/PlayerLocomotionComponent.h"

#include "Engine/ECS/Components/StateMachineComponent.h"
#include "Engine/ECS/Systems/StateMachineSystem.h"

PlayerIdleSystem::PlayerIdleSystem()
{
	StateMachineSystem::RequireState<PlayerIdleComponent>("Idle", "PlayerIdleSystem");
}

void PlayerIdleSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	(void)elapsedTime;
	(void)context;

	auto view = registry.view<PlayerIdleComponent, StateMachineComponent, PlayerLocomotionComponent>();
	view.each([&](const PlayerIdleComponent& idle, const StateMachineComponent& stateMachine, PlayerLocomotionComponent& loco)
		{
			if (!stateMachine.IsIn("Idle")) return;

			// その場で止まる(向きは変えない)
			LocomotionCommand& cmd = loco.command;
			cmd.mode = LocomotionMode::Ground;
			cmd.moveDir = { 0.0f, 0.0f, 0.0f };
			cmd.targetSpeed = 0.0f;
			cmd.decelHalfLife = idle.stopHalfLife;
			cmd.turnSpeed = 0.0f;
		});
}