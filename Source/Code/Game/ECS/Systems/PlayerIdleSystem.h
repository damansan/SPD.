#pragma once
#include "entt.hpp"
#include "Engine/Core/SceneContext.h"

class PlayerIdleSystem
{
public:
	PlayerIdleSystem();
	void Update(entt::registry& registry, float elapsedTime, const SceneContext& context);
};