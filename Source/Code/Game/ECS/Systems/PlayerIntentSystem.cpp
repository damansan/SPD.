#include "PlayerIntentSystem.h"
#include <algorithm>

#include "Game/ECS/Components/GameInputComponent.h"
#include "Game/ECS/Components/PlayerIntentComponent.h"
#include "Game/ECS/Systems/PlayerMovementUtils.h"

namespace
{
	// ボタン1つぶんの意図を更新する
	void UpdateAction(ActionIntent& action, bool pressed, bool held, float bufferTime, float elapsedTime)
	{
		// 押し続けている時間
		action.held = held;
		action.heldTime = held ? action.heldTime + elapsedTime : 0.0f;

		// 先行入力：押された瞬間に受け付け、受付時間を過ぎたら取り消す
		if (pressed)
		{
			action.buffered = true;
			action.bufferTimer = 0.0f;
		}
		else if (action.buffered)
		{
			action.bufferTimer += elapsedTime;
			if (action.bufferTimer > bufferTime)
			{
				action.buffered = false;
			}
		}
	}
}

void PlayerIntentSystem::Update(entt::registry& registry, float elapsedTime, const SceneContext& context)
{
	// カメラがない場合は処理しない
	if (!context.camera)return;

	// カメラの前方向と右方向を取得
	const DirectX::XMFLOAT3 front = context.camera->GetFront();
	const DirectX::XMFLOAT3 right = context.camera->GetRight();

	auto view = registry.view<GameInputComponent, PlayerIntentComponent>();
	view.each([&](const GameInputComponent& input, PlayerIntentComponent& intent)
		{
			// 生の入力を、カメラ基準のワールド方向に変換
			auto dir = PlayerMovementUtils::StickToWorldVelocity(input.moveStick.x, input.moveStick.y, front, right, 1.0f);
			intent.hasRawInput = (dir.inputLength > 0.0f);

			// 動きたいか(猶予付き)
			if (intent.hasRawInput)
			{
				// 入力あり：方向と倒し具合を更新して、猶予をリセット
				intent.moveDir = dir.velocity; // 長さ1
				intent.moveAmount = (std::min)(dir.inputLength, 1.0f);
				intent.graceTimer = 0.0f;
				intent.wantsMove = true;
			}
			else
			{
				// 入力なし：猶予の間は、最後の方向と倒し具合を保つ
				intent.graceTimer += elapsedTime;
				if (intent.graceTimer >= intent.moveGraceTime)
				{
					intent.wantsMove = false;
					intent.moveAmount = 0.0f;
				}
			}

			// 走りたいか(ヒステリシス＋猶予)
			if (!intent.wantsMove)
			{
				// 動きたくない：猶予なしで止める
				intent.wantsRun = false;
				intent.runGraceTimer = 0.0f;
			}
			else if (intent.moveAmount >= intent.runEnterThreshold)
			{
				// 深く倒した：走る。猶予を測り直す
				intent.wantsRun = true;
				intent.runGraceTimer = 0.0f;
			}
			else if (intent.moveAmount < intent.runExitThreshold)
			{
				// 浅くなった：猶予の時間だけ待ってから歩きに戻す
				intent.runGraceTimer += elapsedTime;
				if (intent.runGraceTimer >= intent.runGraceTime)
				{
					intent.wantsRun = false;
				}
			}
			else
			{
				// その間：今の状態を保つ
				intent.runGraceTimer = 0.0f;
			}

			// ボタンの意図
			UpdateAction(intent.jump, input.jump.pressed, input.jump.held, intent.jumpBufferTime, elapsedTime);
			UpdateAction(intent.dodge, input.dodge.pressed, input.dodge.held, intent.dodgeBufferTime, elapsedTime);
		});
}