#pragma once
#include "entt.hpp"
#include "EditorContext.h"

class InspectorWindow
{
public:
    // 描画
    void Draw(entt::registry& registry, EditorContext& context);

private:
    // 名前編集UI
    void DrawNameField(entt::registry& registry, EditorContext& context);

    // コンポーネント一覧UI
    void DrawComponents(entt::registry& registry, EditorContext& context);

	// コンポーネント追加UI
    void DrawAddComponent(entt::registry& registry, EditorContext& context);
};