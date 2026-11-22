#pragma once
#include "entt.hpp"
#include "Engine/Core/Camera.h"

class CameraViewSystem
{
public:
	// 最終の値を作る
	void Update(entt::registry& registry);

	// Camera に渡す
	void ApplyToCamera(entt::registry& registry, Camera& camera);
};