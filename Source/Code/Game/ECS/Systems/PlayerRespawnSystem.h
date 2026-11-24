#pragma once
#pragma once
#include "entt.hpp"
#include "Game/ECS/Components/PlayerRespawnComponent.h"

// 落下したら、黒くして開始位置へ戻す
class PlayerRespawnSystem
{
public:
	void Update(entt::registry& registry, float elapsedTime);
};