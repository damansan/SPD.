#pragma once
#include "Engine/Rendering/PostProcessSettings.h"

// ポストエフェクト調整ツール
class PostProcessToolWindow
{
public:
	// 調整画面を描く
	void Draw(PostProcessSettings& settings);

	// 表示しているか
	bool isOpen = false;
};