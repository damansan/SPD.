#include "PlayerCrouchSystem.h"
#include "Game/ECS/Components/PlayerIntentComponent.h"
#include "Game/ECS/Components/PlayerCrouchComponent.h"
#include "Game/ECS/Components/PlayerLocomotionComponent.h"

#include "Engine/ECS/Components/StateMachineComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Components/CharacterContactComponent.h"
#include "Engine/ECS/Systems/StateMachineSystem.h"

PlayerCrouchSystem::PlayerCrouchSystem()
{
	StateMachineSystem::RequireState<PlayerCrouchComponent>("Crouch", "PlayerCrouchSystem");
}

void PlayerCrouchSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	(void)elapsedTime;
	(void)context;

	auto view = registry.view<PlayerIntentComponent, PlayerCrouchComponent, StateMachineComponent, PlayerLocomotionComponent, MovementComponent, CharacterContactComponent>();
	view.each([&](PlayerIntentComponent& intent, PlayerCrouchComponent& crouch,
		const StateMachineComponent& stateMachine, PlayerLocomotionComponent& loco, MovementComponent& movement,
		const CharacterContactComponent& contact)
		{
			if (!stateMachine.IsIn("Crouch")) return;

			if (stateMachine.enteredThisFrame)
			{
				// 入った瞬間：入ってきた ○ を使い切る(残っていると、立つ判定にも使われてしまう)
				intent.dodge.Consume();
				crouch.wantsStand = false;
			}
			else if (intent.dodge.buffered)
			{
				// しゃがみ中に ○：立ちたい。使い切る(立った後に、また Crouch に入らないように)
				crouch.wantsStand = true;
				intent.dodge.Consume();
			}

			// 立ちたい かつ 頭の上が空いている → 立つ(天井の下なら、出るまで待つ)
			crouch.standRequested = crouch.wantsStand && contact.canStand;

			// 意図を、Locomotion への指示に変える
			LocomotionCommand& cmd = loco.command;
			cmd.mode = LocomotionMode::Ground;
			cmd.moveDir = intent.moveDir;
			cmd.targetSpeed = intent.wantsMove ? crouch.walkSpeed : 0.0f;
			cmd.accelHalfLife = crouch.accelHalfLife;
			cmd.decelHalfLife = crouch.decelHalfLife;
			cmd.turnSpeed = crouch.turnSpeed;

			// 当たり判定を低くする指示
			movement.heightScale = crouch.heightScale;
		});
}