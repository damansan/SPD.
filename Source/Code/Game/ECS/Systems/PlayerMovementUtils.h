#pragma once
#include <DirectXMath.h>

// プレイヤーの移動計算
namespace PlayerMovementUtils
{
	// スティック入力とカメラの前方/右方向から、ワールド座標系の速度ベクトルと、
	// スティックの倒し具合を求める
	struct WorldVelocityResult
	{
		DirectX::XMFLOAT3 velocity{ 0.0f, 0.0f, 0.0f };
		float inputLength = 0.0f; // スティックの倒し具合
	};

	WorldVelocityResult StickToWorldVelocity(
		float inputX, float inputZ,
		const DirectX::XMFLOAT3& cameraFront,
		const DirectX::XMFLOAT3& cameraRight,
		float speed);

	WorldVelocityResult ResolvePlanarVelocity(
		float inputX, float inputZ,
		const DirectX::XMFLOAT3& cameraFront,
		const DirectX::XMFLOAT3& cameraRight,
		float targetSpeed,
		const DirectX::XMFLOAT3& currentVelocity,
		float elapsedTime,
		float acceleration = -1.0f);

	// ワールド座標系の移動方向から、Y軸回転角を求める
	float CalcFacingYaw(float worldDirX, float worldDirZ);

	// 滑らかに回転させる
	float RotateTowards(float currentYaw, float targetYaw, float turnSpeed, float elapsedTime);

	// fromYaw から toYaw への角度差
	float AngleDiff(float fromYaw, float toYaw);

	// Yaw から水平方向の前方ベクトルを求める
	DirectX::XMFLOAT3 YawToForward(float yaw);

	// current を target へ、最大 maxStep だけ近づける
	float MoveTowards(float current, float target, float maxStep);

	// current を target へ、指数的に近づける(halfLife 秒で差が半分になる。フレームレート非依存)
	float Damper(float current, float target, float halfLife, float elapsedTime);
}