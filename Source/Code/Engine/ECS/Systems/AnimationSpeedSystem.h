#pragma once
#include "entt.hpp"
#include "Engine/Data/AnimationSpeedData.h"
#include "Engine/Core/JsonFileCache.h"
#include <string>

class AnimationSpeedSystem
{
public:
	void Update(entt::registry& registry);

private:
	JsonFileCache<AnimationSpeedData> dataCache;
};