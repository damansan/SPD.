#pragma once
#include "entt.hpp"
#include "Engine/Core/SceneContext.h"

// コントローラーの生入力をプレイヤーの意図に解釈する
class PlayerIntentSystem
{
public:
	void Update(entt::registry& registry, float elapsedTime, const SceneContext& context);
};