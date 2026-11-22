#pragma once
#include "entt.hpp"
#include "Engine/Core/SceneContext.h"

class PlayerCrouchSystem
{
public:
	PlayerCrouchSystem();
	void Update(entt::registry& registry, float elapsedTime, const SceneContext& context);
};