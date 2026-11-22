#include "MovementSystem.h"

#include "Engine/ECS/Components/MovementComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Components/PhysicsComponents.h"

#include "entt.hpp"

void MovementSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
    auto view = registry.view<MovementComponent, TransformComponent>();
    view.each([&](auto entity,
        MovementComponent& movement,
        TransformComponent& transform)
        {
            // 重力
            movement.velocity.y += movement.gravity * movement.gravityScale * elapsedTime;
            movement.gravityScale = 1.0f;
        });
}
