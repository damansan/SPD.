#pragma once
#include <imgui.h>
#include <vector>
#include "Engine/Core/Log.h"

// ログの表示ウィンドウ
class LogWindow
{
public:
	void Draw();

private:
	std::vector<LogBuffer::Entry> cache; // 表示用の写し
	uint64_t cachedVersion = 0;
	ImGuiTextFilter filter;
};