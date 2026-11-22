#pragma once
#include "entt.hpp"
#include "Engine/Core/Camera.h"
#include "Engine/ECS/Components/FreeCameraComponent.h"

class DebugCameraSystem
{
public:
    // フリーカメラの更新
    void Update(entt::registry& registry, float elapsedTime, bool viewportHovered);

    // Camera クラスに結果を反映
    void ApplyToCamera(entt::registry& registry, Camera& camera);

private:
    // フリーカメラ更新処理
    void UpdateFreeCamera(FreeCameraComponent& free, float elapsedTime, bool viewportHovered);
};