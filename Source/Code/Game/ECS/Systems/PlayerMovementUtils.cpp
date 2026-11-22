#include "PlayerMovementUtils.h"
#include <cmath>

namespace PlayerMovementUtils
{
	WorldVelocityResult StickToWorldVelocity(
		float inputX, float inputZ,
		const DirectX::XMFLOAT3& cameraFront,
		const DirectX::XMFLOAT3& cameraRight,
		float maxSpeed)
	{
		WorldVelocityResult result;

		float frontLen = sqrtf(cameraFront.x * cameraFront.x + cameraFront.z * cameraFront.z);
		float rightLen = sqrtf(cameraRight.x * cameraRight.x + cameraRight.z * cameraRight.z);

		float frontX = (frontLen > 0.0f) ? cameraFront.x / frontLen : 0.0f;
		float frontZ = (frontLen > 0.0f) ? cameraFront.z / frontLen : 0.0f;
		float rightX = (rightLen > 0.0f) ? cameraRight.x / rightLen : 0.0f;
		float rightZ = (rightLen > 0.0f) ? cameraRight.z / rightLen : 0.0f;

		float dirX = frontX * inputZ + rightX * inputX;
		float dirZ = frontZ * inputZ + rightZ * inputX;
		float len = sqrtf(dirX * dirX + dirZ * dirZ);

		result.inputLength = len;
		
		if (len > 0.0f)
		{
			result.velocity.x = (dirX / len) * maxSpeed;
			result.velocity.z = (dirZ / len) * maxSpeed;
		}

		return result;
	}

	WorldVelocityResult ResolvePlanarVelocity(
		float inputX, float inputZ,
		const DirectX::XMFLOAT3& cameraFront,
		const DirectX::XMFLOAT3& cameraRight,
		float targetSpeed,
		const DirectX::XMFLOAT3& currentVelocity,
		float elapsedTime,
		float acceleration)
	{
		// 目標(方向は入力方向、大きさは targetSpeed)
		WorldVelocityResult result = StickToWorldVelocity(inputX, inputZ, cameraFront, cameraRight, targetSpeed);

		// 負なら従来どおり、即座に目標速度
		if (acceleration < 0.0f) return result;
		if (targetSpeed <= 0.0f) return result;

		// 現在の水平方向の速さ
		float currentSpeed = sqrtf(currentVelocity.x * currentVelocity.x + currentVelocity.z * currentVelocity.z);

		// 速さだけを、1フレームあたり最大 acceleration * elapsedTime ずつ目標へ近づける
		float maxStep = acceleration * elapsedTime;
		float diff = targetSpeed - currentSpeed;
		float newSpeed = (fabsf(diff) <= maxStep)
			? targetSpeed
			: currentSpeed + (diff > 0.0f ? maxStep : -maxStep);

		// 方向は入力方向のまま、大きさだけ差し替える
		float scale = newSpeed / targetSpeed;
		result.velocity.x *= scale;
		result.velocity.z *= scale;

		return result;
	}

	float CalcFacingYaw(float worldDirX, float worldDirZ)
	{
		return atan2f(-worldDirX, -worldDirZ);
	}
	float RotateTowards(float currentYaw, float targetYaw, float turnSpeed, float elapsedTime)
	{
		float diff = targetYaw - currentYaw;
		while (diff > DirectX::XM_PI)  diff -= DirectX::XM_2PI;
		while (diff < -DirectX::XM_PI) diff += DirectX::XM_2PI;

		float maxStep = turnSpeed * elapsedTime;
		if (diff > maxStep)       diff = maxStep;
		else if (diff < -maxStep) diff = -maxStep;

		float result = currentYaw + diff;

		while (result > DirectX::XM_PI)  result -= DirectX::XM_2PI;
		while (result < -DirectX::XM_PI) result += DirectX::XM_2PI;

		return result;
	}

	float AngleDiff(float fromYaw, float toYaw)
	{
		float diff = toYaw - fromYaw;
		while (diff > DirectX::XM_PI)  diff -= DirectX::XM_2PI;
		while (diff < -DirectX::XM_PI) diff += DirectX::XM_2PI;
		return diff;
	}

	DirectX::XMFLOAT3 YawToForward(float yaw)
	{
		// CalcFacingYaw が atan2(-x, -z) なので、その逆
		return { -sinf(yaw), 0.0f, -cosf(yaw) };
	}

	float MoveTowards(float current, float target, float maxStep)
	{
		float diff = target - current;
		if (fabsf(diff) <= maxStep) return target;
		return current + (diff > 0.0f ? maxStep : -maxStep);
	}

	float Damper(float current, float target, float halfLife, float elapsedTime)
	{
		if (halfLife <= 0.0f) return target;
		// 経過時間が halfLife 1つ分なら、差は 2^-1 = 半分になる
		return target + (current - target) * exp2f(-elapsedTime / halfLife);
	}
}