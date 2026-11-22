#pragma once
#include <DirectXMath.h>
#include "Engine/ECS/Reflect.h"

// ボタン1つぶんの意図
struct ActionIntent
{
	REFLECT_BEGIN(ActionIntent, "Action Intent")

		REFLECT_FIELD(bool,  buffered, (false), ("Buffered",  0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 先行入力として受け付け中
		REFLECT_FIELD(bool,  held,     (false), ("Held",      0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 今押し続けているか
		REFLECT_FIELD(float, heldTime, (0.0f),  ("Held Time", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 押し続けている時間

		REFLECT_END()

		// 内部実装用
		float bufferTimer = 0.0f; // 押してからの経過時間

	// ステートが使った時に呼ぶ
	void Consume() { buffered = false; }
};

// プレイヤーの意図
struct PlayerIntentComponent
{
	REFLECT_BEGIN(PlayerIntentComponent, "Player Intent")

		// 設定値
		REFLECT_FIELD(float, moveGraceTime,     (0.1f),  ("Move Grace Time",     0.0f, 0.5f)) // 入力が途切れても wantsMove を保つ時間
		REFLECT_FIELD(float, runEnterThreshold, (0.9f),  ("Run Enter Threshold", 0.0f, 1.0f)) // この倒し具合以上で走りに入る
		REFLECT_FIELD(float, runExitThreshold,  (0.7f),  ("Run Exit Threshold",  0.0f, 1.0f)) // この倒し具合未満で歩きに戻る
		REFLECT_FIELD(float, runGraceTime,      (0.15f), ("Run Grace Time",      0.0f, 0.5f)) // 倒し具合が戻っても wantsRun を保つ時間
		REFLECT_FIELD(float, jumpBufferTime,    (0.15f), ("Jump Buffer Time",    0.0f, 0.5f)) // ジャンプの先行入力を受け付けておく時間
		REFLECT_FIELD(float, dodgeBufferTime,   (0.15f), ("Dodge Buffer Time",   0.0f, 0.5f)) // 回避の先行入力を受け付けておく時間

		// 意図
		REFLECT_FIELD(bool, wantsMove, (false), ("Wants Move", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 動きたいか
		REFLECT_FIELD(bool, wantsRun,  (false), ("Wants Run",  0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 走りたいか
		
		REFLECT_FIELD(DirectX::XMFLOAT3, moveDir, (0.0f, 0.0f, 0.0f), ("Move Dir", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // ワールド方向
		
		REFLECT_FIELD(float, moveAmount,  (0.0f),  ("Move Amount",   0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 倒し具合
		REFLECT_FIELD(bool,  hasRawInput, (false), ("Has Raw Input", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 今フレームの生の入力があるか

		// ボタンの意図
		REFLECT_FIELD(ActionIntent, jump, (), ("Jump"))
		REFLECT_FIELD(ActionIntent, dodge, (), ("Dodge"))

		REFLECT_END()

		// 内部実装用
		float graceTimer = 0.0f; // 入力が途切れてからの経過時間
		float runGraceTimer = 0.0f; // 倒し具合が runExitThreshold を下回ってからの経過時間
};