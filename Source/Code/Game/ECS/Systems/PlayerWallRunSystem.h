#pragma once
#include "entt.hpp"
#include "Engine/Core/SceneContext.h"

class PlayerWallRunSystem
{
public:
	PlayerWallRunSystem();
	void Update(entt::registry& registry, float elapsedTime, const SceneContext& context);
};