#pragma once
#include "entt.hpp"
#include "Engine/Core/SceneContext.h"

class PlayerSlideSystem
{
public:
	PlayerSlideSystem();
	void Update(entt::registry& registry, float elapsedTime, const SceneContext& context);
};