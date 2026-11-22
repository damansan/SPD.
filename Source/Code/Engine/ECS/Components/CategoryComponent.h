#pragma once
#include "Engine/ECS/Reflect.h"

// エンティティの属性を付与するコンポーネント
enum class EntityCategory
{
    None, Player, Enemy, Stage, Object, Camera
};

REFLECT_ENUM(EntityCategory)

    REFLECT_ENUM_VALUE(None,   EntityCategory::None,   "None")
    REFLECT_ENUM_VALUE(Player, EntityCategory::Player, "Player")
    REFLECT_ENUM_VALUE(Enemy,  EntityCategory::Enemy,  "Enemy")
    REFLECT_ENUM_VALUE(Stage,  EntityCategory::Stage,  "Stage")
    REFLECT_ENUM_VALUE(Object, EntityCategory::Object, "Object")
    REFLECT_ENUM_VALUE(Camera, EntityCategory::Camera, "Camera")

REFLECT_ENUM_END()

struct CategoryComponent
{
    REFLECT_BEGIN(CategoryComponent, "Category")
        
        REFLECT_FIELD(EntityCategory, category, (EntityCategory::None), ("Category"))

    REFLECT_END()
};