#pragma once
#include "Engine/ECS/Reflect.h"

struct PlayerCrouchComponent
{
	REFLECT_BEGIN(PlayerCrouchComponent, "Player Crouch")

		REFLECT_FIELD(float, walkSpeed,      (1.2f), ("Walk Speed",      0.0f, 10.0f)) // しゃがみ歩きの速さ
		REFLECT_FIELD(float, accelHalfLife, (0.08f), ("Accel Half Life", 0.0f, 1.0f))  // 加速時の半減期
		REFLECT_FIELD(float, decelHalfLife, (0.08f), ("Decel Half Life", 0.0f, 1.0f))  // 減速時の半減期
		REFLECT_FIELD(float, turnSpeed,      (8.0f), ("Turn Speed",      0.0f, 30.0f)) // 旋回速度
		REFLECT_FIELD(float, heightScale, (0.5f), ("Height Scale", 0.1f, 1.0f))        // しゃがみ中の当たり判定の高さ
		
		// 状態
		REFLECT_FIELD(bool, wantsStand, (false), ("Wants Stand", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 立ちたい
		REFLECT_FIELD(bool, standRequested, (false), ("Stand Requested", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 立ちたい かつ 立てる

		REFLECT_END()
};