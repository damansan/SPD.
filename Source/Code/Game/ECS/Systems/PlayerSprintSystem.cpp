#include "PlayerSprintSystem.h"
#include "Game/ECS/Components/PlayerIntentComponent.h"
#include "Game/ECS/Components/PlayerSprintComponent.h"
#include "Game/ECS/Components/PlayerLocomotionComponent.h"

#include "Engine/ECS/Components/StateMachineComponent.h"
#include "Engine/ECS/Systems/StateMachineSystem.h"

PlayerSprintSystem::PlayerSprintSystem()
{
	StateMachineSystem::RequireState<PlayerSprintComponent>("Sprint", "PlayerSprintSystem");
}

void PlayerSprintSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	(void)elapsedTime;
	(void)context;

	auto view = registry.view<PlayerIntentComponent, PlayerSprintComponent, StateMachineComponent, PlayerLocomotionComponent>();
	view.each([&](const PlayerIntentComponent& intent, const PlayerSprintComponent& sprint,
		const StateMachineComponent& stateMachine, PlayerLocomotionComponent& loco)
		{
			if (!stateMachine.IsIn("Sprint")) return;

			// 意図を、Locomotion への指示に変える
			LocomotionCommand& cmd = loco.command;
			cmd.mode = LocomotionMode::Ground;
			cmd.moveDir = intent.moveDir;
			cmd.targetSpeed = sprint.sprintSpeed;
			cmd.accelHalfLife = sprint.accelHalfLife;
			cmd.decelHalfLife = sprint.decelHalfLife;
			cmd.turnSpeed = sprint.turnSpeed;
		});
}