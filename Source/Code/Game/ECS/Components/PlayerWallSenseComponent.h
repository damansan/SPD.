#pragma once
#include "Engine/ECS/Reflect.h"

struct PlayerWallSenseComponent
{
	REFLECT_BEGIN(PlayerWallSenseComponent, "Player Wall Sense")

		// 設定値
		REFLECT_FIELD(float, maxWallNormalY, (0.3f), ("Max Wall Normal Y", 0.0f, 1.0f)) // 法線の y の大きさがこれ以下なら壁とみなす
		REFLECT_FIELD(float, wallGraceTime, (0.15f), ("Wall Grace Time", 0.0f, 1.0f))   // 壁から離れても nearWall を保つ時間

		// 状態
		REFLECT_FIELD(bool, touchingWall, (false), ("Touching Wall", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 壁に当たっているか
		REFLECT_FIELD(float, approachAngleDeg, (0.0f), ("Approach Angle Deg", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 当たる角度(0=かすめる、90=正面、マイナス=離れていく)
		REFLECT_FIELD(bool, isWallRight, (false), ("Is Wall Right", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 進む向きから見て、壁が右にあるか

		// 猶予付き
		REFLECT_FIELD(float, wallLostTime, (999.0f), ("Wall Lost Time", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 壁から離れてからの時間
		REFLECT_FIELD(bool,  nearWall,      (false), ("Near Wall",      0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 壁にいる(猶予付き)

		REFLECT_END()
};