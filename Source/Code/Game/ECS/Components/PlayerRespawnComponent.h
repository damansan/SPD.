#pragma once
#include <DirectXMath.h>
#include "Engine/ECS/Reflect.h"

// 落下からの復帰
struct PlayerRespawnComponent
{
	REFLECT_BEGIN(PlayerRespawnComponent, "Player Respawn")

		// 設定
		REFLECT_FIELD(float, killY, (-20.0f), ("Kill Y", -1000.0f, 1000.0f)) // これより下で復帰
		REFLECT_FIELD(float, fadeOutTime, (0.6f), ("Fade Out Time", 0.01f, 5.0f)) // 黒くなるまでの時間
		REFLECT_FIELD(float, fadeInTime, (0.6f), ("Fade In Time", 0.01f, 5.0f)) // 戻るまでの時間
		REFLECT_FIELD(float, blackTime, (0.5f), ("Black Time", 0.0f, 5.0f)) // 真っ暗で待つ時間

		// 状態
		REFLECT_FIELD(bool, respawning, (false), ("Respawning", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // フェードアウト中
		REFLECT_FIELD(float, fade, (0.0f), ("Fade", 0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 0=通常 1=真っ黒
		REFLECT_FIELD(float, blackTimer, (0.0f), ("Black Timer", 0.0f, 0.0f, 0.01f, "", FieldFlags::VisibleOnly | FieldFlags::Transient)) // 真っ暗の残り時間

		REFLECT_END()

		// 戻る場所（保存しない）
		DirectX::XMFLOAT3 spawnPosition = { 0.0f, 0.0f, 0.0f };
	bool hasSpawn = false;
};