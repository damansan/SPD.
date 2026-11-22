#pragma once
#include "entt.hpp"
#include "Engine/Core/SceneContext.h"

class PlayerDodgeSystem
{
public:
	PlayerDodgeSystem();
	void Update(entt::registry& registry, float elapsedTime, const SceneContext& context);
};