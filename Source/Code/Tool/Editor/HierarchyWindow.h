#pragma once
#include "entt.hpp"
#include "Tool/Editor/EditorContext.h"

class HierarchyWindow
{
public:
    // 描画
    void Draw(entt::registry& registry, EditorContext& context);
};