#pragma once
#include <DirectXMath.h>
#include "Engine/ECS/Reflect.h"

// 回避の方向
enum class PlayerDodgeDirection
{
	Front,
	Back,
	Left,
	Right,
};

struct PlayerDodgeComponent
{
	REFLECT_BEGIN(PlayerDodgeComponent, "Player Dodge")

		// 設定値
		REFLECT_FIELD(float, dodgeSpeed, (8.0f), ("Dodge Speed", 0.0f, 20.0f)) // 回避中の速さ
		REFLECT_FIELD(float, minTime, (0.15f), ("Min Time", 0.0f, 2.0f))  // R2を短く押した時の長さ
		REFLECT_FIELD(float, maxTime, (0.5f), ("Max Time", 0.0f, 2.0f))  // R2を押し続けた時の長さ
		REFLECT_FIELD(float, accelHalfLife, (0.02f), ("Accel Half Life", 0.0f, 1.0f))  // 回避の速さに達するまでの半減期
		REFLECT_FIELD(float, turnSpeed, (10.0f), ("Turn Speed", 0.0f, 30.0f)) // 回避方向を曲げる速さ

		// 状態
		REFLECT_FIELD(bool, isFront,  (false), ("Is Front", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 前へ回避
		REFLECT_FIELD(bool, isBack,   (false), ("Is Back",  0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 後ろへ回避
		REFLECT_FIELD(bool, isLeft,   (false), ("Is Left",  0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 左へ回避
		REFLECT_FIELD(bool, isRight,  (false), ("Is Right", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 右へ回避
		REFLECT_FIELD(bool, finished, (false), ("Finished", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 回避が終わったか
		REFLECT_FIELD(bool, faceDodgeDir, (false), ("Face Dodge Dir", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 回避方向を向いて回避するか(前方3方向のとき)

		// 体から見た回避方向
		REFLECT_FIELD(float, localX,  (0.0f), ("Local X", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 右が +
		REFLECT_FIELD(float, localZ,  (0.0f), ("Local Z", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 前が +
		REFLECT_FIELD(float, elapsed, (0.0f), ("Elapsed", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 回避に入ってからの時間(秒)

		REFLECT_END()

		// 内部実装用
		PlayerDodgeDirection direction = PlayerDodgeDirection::Back; // 入った時に決めた方向
	DirectX::XMFLOAT3    dodgeDir = { 0.0f, 0.0f, 0.0f };        // 今進んでいる方向
};