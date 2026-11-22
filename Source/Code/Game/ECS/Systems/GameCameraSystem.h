#pragma once
#include "entt.hpp"
#include "Engine/Core/Camera.h"
#include "Game/ECS/Components/GameCameraComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"

class GameCameraSystem
{
public:
    // 更新
    void Update(entt::registry& registry, float elapsedTime);

    // Camera クラスに結果を反映
    void ApplyToCamera(entt::registry& registry, Camera& camera);

private:
    // 追従カメラの処理
    void UpdateFollow(GameCameraComponent& cam, const TransformComponent& followTransform, float elapsedTime);

    // 追従カメラの理想の eye・target
    DirectX::XMFLOAT3 CalcFollowIdealEye(const GameCameraComponent& cam, const TransformComponent& followTransform) const;
    DirectX::XMFLOAT3 CalcFollowIdealTarget(const GameCameraComponent& cam, const TransformComponent& followTransform) const;
};