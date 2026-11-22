#pragma once
#pragma once
#include "entt.hpp"
#include "Engine/Core/SceneContext.h"

// 壁に当たった結果と速度から、当たる角度・左右を求める
class PlayerWallSenseSystem
{
public:
	void Update(entt::registry& registry, float elapsedTime, const SceneContext& context);
};