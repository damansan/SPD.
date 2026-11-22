#include "GameComponents.h"
#include "Engine/Resource/ComponentRegistry.h"

#include "Game/ECS/Components/GameInputComponent.h"
#include "Game/ECS/Components/GameCameraComponent.h"
#include "Game/ECS/Components/PlayerIdleComponent.h"
#include "Game/ECS/Components/PlayerMoveComponent.h"
#include "Game/ECS/Components/PlayerDodgeComponent.h"
#include "Game/ECS/Components/PlayerSprintComponent.h"
#include "Game/ECS/Components/PlayerJumpComponent.h"
#include "Game/ECS/Components/PlayerWallRunComponent.h"
#include "Game/ECS/Components/PlayerCrouchComponent.h"
#include "Game/ECS/Components/PlayerSlideComponent.h"

#include "Game/ECS/Components/CameraEffectComponent.h"
#include "Game/ECS/Components/CameraViewComponent.h"

#include "Game/ECS/Components/PlayerIntentComponent.h"
#include "Game/ECS/Components/PlayerLocomotionComponent.h"
#include "Game/ECS/Components/PlayerWallSenseComponent.h"

void GameComponents::Register() 
{
	REGISTER_COMPONENT_TYPE(GameInputComponent,    "GameInput");
	REGISTER_COMPONENT_TYPE(GameCameraComponent,   "GameCamera");
	REGISTER_COMPONENT_TYPE(PlayerIdleComponent,   "Player Idle");
	REGISTER_COMPONENT_TYPE(PlayerMoveComponent,   "Player Move");
	REGISTER_COMPONENT_TYPE(PlayerDodgeComponent,  "Player Dodge");
	REGISTER_COMPONENT_TYPE(PlayerSprintComponent, "Player Sprint");
	REGISTER_COMPONENT_TYPE(PlayerJumpComponent,   "Player Jump");
	REGISTER_COMPONENT_TYPE(PlayerWallRunComponent, "Player Wall Run");
	REGISTER_COMPONENT_TYPE(PlayerCrouchComponent, "Player Crouch");
	REGISTER_COMPONENT_TYPE(PlayerSlideComponent, "Player Slide");

	REGISTER_COMPONENT_TYPE(CameraEffectComponent, "Camera Effect");
	REGISTER_COMPONENT_TYPE(CameraViewComponent, "Camera View");

	REGISTER_COMPONENT_TYPE(PlayerIntentComponent, "Player Intent");
	REGISTER_COMPONENT_TYPE(PlayerLocomotionComponent, "Player Locomotion");
	REGISTER_COMPONENT_TYPE(PlayerWallSenseComponent, "Player Wall Sense");

}