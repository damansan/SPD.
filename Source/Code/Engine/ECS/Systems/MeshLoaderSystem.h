#pragma once
#include <d3d11.h>

#include "entt.hpp"

#include "Engine/ECS/Components/MeshComponent.h"
#include "Engine/ECS/Components/StaticMeshComponent.h"
#include "Engine/ECS/Components/SkinnedMeshComponent.h"
#include "Engine/ECS/Components/Tags.h"

#include "Engine/Resource/ResourceManager.h"

class MeshLoaderSystem
{
public:
    // 更新(未ロードのMeshComponentを検出してロード）
    void Update(
        entt::registry& registry,
        ID3D11Device* device,
        ResourceManager& resourceManager
    );
};