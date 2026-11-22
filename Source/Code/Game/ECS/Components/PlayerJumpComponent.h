#pragma once
#include "Engine/ECS/Reflect.h"

struct PlayerJumpComponent
{
	REFLECT_BEGIN(PlayerJumpComponent, "Player Jump")

		// 設定値
		REFLECT_FIELD(float, jumpSpeed, (8.0f), ("Jump Speed", 0.0f, 30.0f)) // 上向きの初速(m/s)。重力 -20 で高さ約1.6m
		REFLECT_FIELD(float, airSpeed, (4.2f), ("Air Speed", 0.0f, 20.0f)) // 空中で入力した時の目標速度の下限(m/s)
		REFLECT_FIELD(float, airControlHalfLife, (0.5f), ("Air Control Half Life", 0.0f, 3.0f))  // 空中で理想の速度に寄る半減期(秒)。大きいほど曲げにくい
		REFLECT_FIELD(float, airTurnSpeed, (5.0f), ("Air Turn Speed", 0.0f, 30.0f)) // 空中の旋回速度(rad/s)

		// 跳び方(実行中)
		REFLECT_FIELD(bool, fromGround, (false), ("From Ground", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 地上から跳んだか(子グラフの入口で使う)

		REFLECT_FIELD(float, airRadius, (0.5f), ("Air Radius", 0.0f, 2.0f)) // 空中の当たり判定の半径

		REFLECT_END()

		// 内部実装用
		float takeoffSpeed = 0.0f; // 踏み切った時の水平の速さ
};