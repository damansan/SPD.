#pragma once
#include "entt.hpp"
#include "Engine/Core/SceneContext.h"

class PlayerSprintSystem
{
public:
	PlayerSprintSystem();
	void Update(entt::registry& registry, float elapsedTime, const SceneContext& context);
};