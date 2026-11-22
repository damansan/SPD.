#pragma once
#include "entt.hpp"
#include "Game/ECS/Components/CameraEffectComponent.h"
#include "Game/ECS/Components/GameCameraComponent.h"

// 演出のきっかけ
struct CameraEffectTrigger
{
	float speed = 0.0f;    // 水平の速さ
	bool  running = false; // 走っている(runSpeed 以上)

	bool wallRunning = false; // 壁走り中
	bool wallKicked = false;  // 壁を蹴った
	bool wallRight = false;   // 壁が右にあるか

	float velocityY = 0.0f;  // 縦の速さ
	bool  isGround = false;  // 着地している

	bool  sliding = false; // スライド中
	float slideSpeed = 0.0f;  // 滑る速さ
};


class CameraEffectSystem
{
public:
	void Update(entt::registry& registry, float elapsedTime);

private:
	// プレイヤーを見て、きっかけを作る
	CameraEffectTrigger BuildTrigger(entt::registry& registry, const CameraEffectComponent& effect, entt::entity player);

	// 速さで FOV を広げる
	void FovEffect(CameraEffectComponent& effect, const CameraEffectTrigger& trigger, float elapsedTime);
	// 走り出しに左右へ揺らす
	void RunBobEffect(CameraEffectComponent& effect, const CameraEffectTrigger& trigger, const GameCameraComponent& cam, float elapsedTime);
	// 壁蹴りで小さく揺らす
	void KickShakeEffect(CameraEffectComponent& effect, const CameraEffectTrigger& trigger, const GameCameraComponent& cam, float elapsedTime);
	// 着地で揺らす
	void LandShakeEffect(CameraEffectComponent& effect, const CameraEffectTrigger& trigger, const GameCameraComponent& cam, float elapsedTime);
	// スライド中は下げ続ける
	void SlideDropEffect(CameraEffectComponent& effect, const CameraEffectTrigger& trigger, float elapsedTime);
	// 壁走り中傾ける
	void WallTiltEffect(CameraEffectComponent& effect, const CameraEffectTrigger& trigger, float elapsedTime);
};