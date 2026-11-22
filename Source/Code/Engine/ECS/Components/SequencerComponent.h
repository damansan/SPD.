#pragma once
#include <string>
#include "Engine/ECS/Reflect.h"

// フラグの種類
enum class SequencerFlag
{
	AnimExit,   // アニメーション終了フラグ


	// 種類が増えたらここに追加していく

	Count
};

struct SequencerComponent
{
	REFLECT_BEGIN(SequencerComponent, "Sequencer")

		// 各フラグ
		REFLECT_FIELD(bool, animExit,   (false), ("Anim Exit",  0.0f, 0.0f, 0.1f, "", FieldFlags::VisibleOnly | FieldFlags::Transient))


	REFLECT_END()
};