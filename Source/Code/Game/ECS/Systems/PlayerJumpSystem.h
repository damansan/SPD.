#pragma once
#include "entt.hpp"
#include "Engine/Core/SceneContext.h"

class PlayerJumpSystem
{
public:
	PlayerJumpSystem();
	void Update(entt::registry& registry, float elapsedTime, const SceneContext& context);
};