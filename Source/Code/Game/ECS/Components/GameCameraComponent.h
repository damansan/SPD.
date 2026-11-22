#pragma once
#include <DirectXMath.h>
#include "entt.hpp"
#include "Engine/ECS/Reflect.h"
#include "Engine/ECS/Components/BaseCameraComponent.h"

// カメラの種類
enum class GameCameraMode
{
	Follow, // 追従カメラ
};

// ゲームカメラコンポーネント
struct GameCameraComponent
{
	// 調整用カメラパラメーター
	REFLECT_BEGIN(GameCameraComponent, "GameCamera")

		// 追従
		REFLECT_FIELD(float, followDistance,  (4.4f), ("Follow Distance",    0.0f, 50.0f))  // プレイヤーからの距離
		REFLECT_FIELD(float, followHeight,    (1.7f), ("Follow Height",      0.0f, 20.0f))  // 目の高さ
		REFLECT_FIELD(float, followTargetYUp, (1.6f), ("Follow Target Y Up", 0.0f, 10.0f))  // 注視点の高さ
		REFLECT_FIELD(float, fov,            (40.0f), ("Fov",               10.0f, 120.0f)) // 基本 FOV

		// 回転
		REFLECT_FIELD(float, rotateSpeed, (2.0f), ("Rotate Speed", 0.0f, 10.0f)) // 回転速度
		REFLECT_FIELD(float, pitchMin,   (-0.8f), ("Pitch Min",   -1.5f, 1.5f))  // 上下の角度の下限
		REFLECT_FIELD(float, pitchMax,    (1.0f), ("Pitch Max",   -1.5f, 1.5f))  // 上下の角度の上限

		// 補間
		REFLECT_FIELD(float, eyeLerpRatio,     (12.0f), ("Eye Lerp Ratio",    0.0f, 100.0f))  // eye の XZ を、毎フレーム残りの何%詰めるか
		REFLECT_FIELD(float, targetLerpRatio,  (24.0f), ("Target Lerp Ratio", 0.0f, 100.0f))  // target の XZ・Y を、毎フレーム残りの何%詰めるか
		REFLECT_FIELD(float, springStiffness, (200.0f), ("Spring Stiffness",  0.0f, 1000.0f)) // eye の Y のばねの強さ
		REFLECT_FIELD(float, springDamping,    (20.0f), ("Spring Damping",    0.0f, 200.0f))  // eye の Y のばねの減衰

		// 状態
		REFLECT_FIELD(float, yaw, (0.0f), ("Yaw", 0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 左右の角度(rad)
		REFLECT_FIELD(float, pitch, (0.3f), ("Pitch", 0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 上下の角度(rad)
	
	REFLECT_END()

	// 土台の出力
	BaseCameraComponent base;

	// カメラの種類
	GameCameraMode mode = GameCameraMode::Follow;

	// 追従対象
	entt::entity followTarget = entt::null;

	bool initialized = false;

	float eyeVelocityY = 0.0f;
};