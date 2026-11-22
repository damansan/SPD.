#pragma once
#include <DirectXMath.h>
#include "Engine/ECS/Reflect.h"


// 演出状態
struct PulseState
{
	REFLECT_BEGIN(PulseState, "Pulse State")

		REFLECT_FIELD(bool, active, (false), ("Active", 0.0f, 0.0f, 0.1f,  "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 再生中
		REFLECT_FIELD(bool, ready,  (true),  ("Ready",  0.0f, 0.0f, 0.1f,  "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 準備完了
		REFLECT_FIELD(float, time,  (0.0f),  ("Time",   0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 経過時間

	REFLECT_END()
};

// カメラの演出
struct CameraEffectComponent
{
	REFLECT_BEGIN(CameraEffectComponent, "Camera Effect")

		// 発火条件
		REFLECT_FIELD(float, runSpeed, (5.0f), ("Run Speed", 0.0f, 50.0f)) // この速さ以上を走りとする

		// Fov：速さで広げる
		REFLECT_FIELD(float, fovAddMax,   (15.0f), ("Fov Add Max", 0.0f, 60.0f))   // 速い時に足す FOV
		REFLECT_FIELD(float, speedFull,   (15.0f), ("Speed Full", 0.0f, 50.0f))    // この速さで FOV を足す
		REFLECT_FIELD(float, fovHalfLife, (0.25f), ("Fov Half Life", 0.01f, 2.0f)) // 足す量を寄せる半減期

		// HeadBob：走り出しに左右へ揺らす
		REFLECT_FIELD(float, runBobRearmSpeed,   (1.0f), ("Run Bob Rearm Speed",   0.0f, 50.0f)) // この速さ以下で準備
		REFLECT_FIELD(float, runBobStepInterval, (0.3f), ("Run Bob Step Interval", 0.05f, 1.0f)) // 片側へ揺れる時間
		REFLECT_FIELD(float, runBobDuration,     (1.5f), ("Run Bob Duration",      0.1f, 5.0f))  // 揺らす時間
		REFLECT_FIELD(float, runBobSway,        (0.15f), ("Run Bob Sway",          0.0f, 1.0f))  // 揺れ幅(横)
		REFLECT_FIELD(float, runBobLift,        (0.05f), ("Run Bob Lift",          0.0f, 1.0f))  // 揺れ幅(縦)
		REFLECT_FIELD(float, runBobRollDeg,      (2.0f), ("Run Bob Roll Deg",      0.0f, 15.0f)) // 傾きの最大

		// KickShake：壁蹴りで1回揺らす
		REFLECT_FIELD(float, kickShakeDuration, (0.3f), ("Kick Shake Duration", 0.05f, 2.0f))  // 寄って戻るまでの時間
		REFLECT_FIELD(float, kickShakeSway,    (0.08f), ("Kick Shake Sway",     0.0f, 0.5f))   // 横ずれの最大
		REFLECT_FIELD(float, kickShakeRollDeg,  (1.5f), ("Kick Shake Roll Deg", 0.0f, 10.0f))  // 傾きの最大

		// LandShake：長く落下したら揺らす
		REFLECT_FIELD(float, landFallSpeed,    (10.0f), ("Land Fall Speed",     0.0f, 50.0f))  // この落下速度以上で準備
		REFLECT_FIELD(float, landShakeDuration, (0.5f), ("Land Shake Duration", 0.05f, 2.0f))  // 揺らす時間
		REFLECT_FIELD(float, landShakeSway,    (0.12f), ("Land Shake Sway",     0.0f, 0.5f))   // 揺れ幅
		REFLECT_FIELD(float, landShakeRollDeg,  (2.0f), ("Land Shake Roll Deg", 0.0f, 10.0f))  // 傾き

		// SlideShake：スライド中は下げる
		REFLECT_FIELD(float, slideDropTime, (0.525f), ("Slide Drop Time", 0.01f, 2.0f)) // 底まで下がる(戻る)時間
		REFLECT_FIELD(float, slideDrop,       (0.5f), ("Slide Drop",      0.0f, 3.0f))  // 下げる深さ

		// WallTilt：壁走り中傾ける
		REFLECT_FIELD(float, wallTiltMaxDeg,       (8.0f), ("Wall Tilt Max Deg",      0.0f, 30.0f))  // 傾きの最大(度)
		REFLECT_FIELD(float, wallTiltHalfLife,     (1.0f), ("Wall Tilt Half Life",    0.01f, 5.0f))  // 傾いていく半減期(秒)。大きいほどじわじわ
		REFLECT_FIELD(float, wallTiltReturnSpeed, (40.0f), ("Wall Tilt Return Speed", 0.0f, 200.0f)) // 戻る速さ


		// 演出ごとの状態
		REFLECT_FIELD(float, fovAdd,        (0.0f), ("Fov Add",         0.0f, 0.0f, 0.1f,  "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // Fov：現在追加している量
		REFLECT_FIELD(float, slideDropRate, (0.0f), ("Slide Drop Rate", 0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // SlideDrop：下がり具合
		REFLECT_FIELD(float, wallTilt,      (0.0f), ("Wall Tilt",       0.0f, 0.0f, 0.1f,  "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // WallTilt：今の傾き

		REFLECT_FIELD(PulseState, runBob,    (), ("Run Bob"))    // HeadBob
		REFLECT_FIELD(PulseState, kickShake, (), ("Kick Shake")) // KickShake
		REFLECT_FIELD(PulseState, landShake, (), ("Land Shake")) // LandShake


		// 出力
		REFLECT_FIELD(DirectX::XMFLOAT3, eyeOffset,    (0.0f, 0.0f, 0.0f), ("Eye Offset",    0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // eye のずれ
		REFLECT_FIELD(DirectX::XMFLOAT3, targetOffset, (0.0f, 0.0f, 0.0f), ("Target Offset", 0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // target のずれ
		REFLECT_FIELD(float, fovOffset, (0.0f), ("Fov Offset", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // FOV の増減
		REFLECT_FIELD(float, roll,      (0.0f), ("Roll",       0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 傾き


		REFLECT_END()
};