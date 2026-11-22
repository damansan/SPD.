#pragma once
#include "entt.hpp"
#include "Engine/ECS/Reflection.h"

class MetaRegistry
{
public:
	// 登録済みメタ情報からInspectorUIを自動描画
	static void DrawMetaFields(entt::meta_type metaType, entt::meta_any instance);
};