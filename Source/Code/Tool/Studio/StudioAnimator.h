#pragma once
#include "IStudioAttachment.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/ECS/Components/SkinnedMeshComponent.h"

class StudioAnimator : public IStudioAttachment
{
public:
    StudioAnimator() = default;
    ~StudioAnimator() = default;

    void SetWindowId(int id) { windowId = id; }

    // ImGui描画
    void DrawGUI(
        entt::registry& registry,
        StudioContext& context) override;

private:
    int windowId = 0;
};