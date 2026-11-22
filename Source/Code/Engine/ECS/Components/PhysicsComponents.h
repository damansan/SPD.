#pragma once
#include <PxPhysicsAPI.h>
#include "Engine/ECS/Reflect.h"

// 剛体の種別
enum class PhysicsBodyType
{
    None,      // 未選択
    Static,    // 静的
    Dynamic,   // 動的
    Character, // キャラクターコントローラー
};

REFLECT_ENUM(PhysicsBodyType)

    REFLECT_ENUM_VALUE(None,      PhysicsBodyType::None,      "None")
    REFLECT_ENUM_VALUE(Static,    PhysicsBodyType::Static,    "Static")
    REFLECT_ENUM_VALUE(Dynamic,   PhysicsBodyType::Dynamic,   "Dynamic")
    REFLECT_ENUM_VALUE(Character, PhysicsBodyType::Character, "Character")

REFLECT_ENUM_END()

// 剛体リスト
struct PhysicsBodyTypeComponent
{
    REFLECT_BEGIN(PhysicsBodyTypeComponent, "Physics")
    
        REFLECT_FIELD(PhysicsBodyType, type, (PhysicsBodyType::None), ("Type"))
    
    REFLECT_END()
};

// 静的剛体コンポーネント
struct StaticBodyComponent
{
    physx::PxRigidStatic* actor = nullptr;
};

// キャラクターコントローラーコンポーネント
struct CharacterControllerComponent
{
    physx::PxController* controller = nullptr;

    // カプセル形状パラメータ
    float radius = 0.2f; // 半径
    float height = 1.6f; // 立っている時の全体の高さ

    // 現在PhysXに設定している値
    float appliedRadius = 0.0f;
    float appliedHeight = 0.0f;

    // 全体の高さと半径から、PhysX に渡す円柱部分の長さを出す
    static float CylinderHeight(float total, float r)
    {
        const float cylinder = total - r * 2.0f;
        return cylinder > 0.01f ? cylinder : 0.01f;
    }
};