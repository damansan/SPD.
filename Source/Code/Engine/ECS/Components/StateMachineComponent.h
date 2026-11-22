#pragma once
#include <string>
#include "Engine/ECS/Reflect.h"

// Stateマシンの状態
struct StateMachineComponent
{
	REFLECT_BEGIN(StateMachineComponent, "State Machine")
		
		REFLECT_FIELD(std::string, currentStateName, (""), ("Current State", 0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient))
		REFLECT_FIELD(std::string, currentClipName,  (""), ("Current Clip",  0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient))
		
	REFLECT_END()

		int currentStateId = -1;  // 現在のState
	int currentClipId = -1;       // 現在再生中のクリップ(子グラフ)
	int enteredTransitionId = -1; // 現在のStateに入ってきた遷移

	// Stateに入った直後で、子グラフのエントリーをまだ処理していないか
	bool pendingEntry = false;

	// このフレームにStateに入ったか
	bool enteredThisFrame = false;

	// 現在のStateが、指定した名前のStateか
	bool IsIn(const std::string& stateName) const { return currentStateName == stateName; }
};