#pragma once
#include <cstdint>

// コントローラーの設定値（JSON保存）
struct ControllerSettings
{
	// スティック デッドゾーン
	float leftStickDeadzone = 0.2f;
	float rightStickDeadzone = 0.2f;

	// トリガー デッドゾーン
	float leftTriggerDeadzone = 0.05f;
	float rightTriggerDeadzone = 0.05f;

	// L2/R2 デジタル判定閾値 
	float leftTriggerPressThreshold = 0.5f;
	float leftTriggerReleaseThreshold = 0.3f;
	float rightTriggerPressThreshold = 0.5f;
	float rightTriggerReleaseThreshold = 0.3f;
};