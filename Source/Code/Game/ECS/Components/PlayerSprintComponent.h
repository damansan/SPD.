#pragma once
#include "Engine/ECS/Reflect.h"

struct PlayerSprintComponent
{
	REFLECT_BEGIN(PlayerSprintComponent, "Player Sprint")

		REFLECT_FIELD(float, sprintSpeed,   (7.4f),  ("Sprint Speed",    0.0f, 20.0f)) // 走る速さ
		REFLECT_FIELD(float, enterSpeed,    (6.0f),  ("Enter Speed",     0.0f, 20.0f)) // 着地の速さがこれ以上なら Sprint に入る
		REFLECT_FIELD(float, accelHalfLife, (0.1f),  ("Accel Half Life", 0.0f, 1.0f))  // 加速時の半減期
		REFLECT_FIELD(float, decelHalfLife, (0.15f), ("Decel Half Life", 0.0f, 1.0f))  // 減速時の半減期
		REFLECT_FIELD(float, turnSpeed,     (8.0f),  ("Turn Speed",      0.0f, 30.0f)) // 旋回速度
		REFLECT_FIELD(float, enterRunTime,  (1.0f),  ("Enter Run Time", 0.0f, 5.0f))   // RunからSprintに入る経過時間
	REFLECT_END()
};