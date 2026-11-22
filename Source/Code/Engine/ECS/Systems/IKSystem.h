#pragma once
#include <DirectXMath.h>
#include "entt.hpp"

#include "Engine/ECS/Components/IKComponent.h"
#include "Engine/ECS/Components/SkinnedMeshComponent.h"
#include "Engine/ECS/Components/TransformComponent.h"

namespace physx { class PxScene; }

class IKSystem
{
public:
    // 更新(AnimationSystemの後、MeshLoaderSystemの前に呼ぶ想定)
    void Update(entt::registry& registry, physx::PxScene* pxScene);

private:
    // 1本の足のFootIK処理
    void SolveFootIK(FootIKSettings& footSettings, SkinnedMeshComponent& skinned, physx::PxScene* pxScene, const DirectX::XMFLOAT4X4& entityWorldMatrix);
};