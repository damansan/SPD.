#pragma once
#include "entt.hpp"

// ステートからの指示と速度を書く
class PlayerLocomotionSystem
{
public:
	void Update(entt::registry& registry, float elapsedTime);
};