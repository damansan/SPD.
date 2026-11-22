#pragma once
#include "entt.hpp"
#include "Engine/Core/SceneContext.h"

class MovementSystem
{
public:
	void Update(entt::registry& registry, float elapsedTime, const SceneContext& context);
};