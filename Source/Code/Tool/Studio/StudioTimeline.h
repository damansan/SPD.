#pragma once
#include "IStudioAttachment.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/ECS/Components/SkinnedMeshComponent.h"

class StudioTimeline : public IStudioAttachment
{
public:
    StudioTimeline() = default;
    ~StudioTimeline() = default;

    void SetWindowId(int id) { windowId = id; }

    void DrawGUI(
        entt::registry& registry,
        StudioContext& context) override;

private:
    int windowId = 0;
};