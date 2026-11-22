#include "LogWindow.h"

void LogWindow::Draw()
{
	ImGui::Begin("Log");

	// ログが更新されていたら、表示用の写しを取り直す
	uint64_t version = LogBuffer::Version();
	if (version != cachedVersion)
	{
		cache = LogBuffer::Snapshot();
		cachedVersion = version;
	}

	filter.Draw("Filter", -50.0f);
	ImGui::Separator();

	// フィルターに合うログの番号だけを集める
	std::vector<int> visible;
	visible.reserve(cache.size());
	for (int i = 0; i < static_cast<int>(cache.size()); ++i)
	{
		if (filter.PassFilter(cache[i].message.c_str()))
		{
			visible.push_back(i);
		}
	}

	ImGui::BeginChild("LogScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

	// 画面に見えている行だけを描画する
	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(visible.size()));
	while (clipper.Step())
	{
		for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
		{
			const auto& entry = cache[visible[row]];
			ImGui::Text("[%s] %s", entry.time.c_str(), entry.message.c_str());
		}
	}
	clipper.End();

	// 一番下まで見ている時は、新しいログに追従する
	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
	{
		ImGui::SetScrollHereY(1.0f);
	}

	ImGui::EndChild();
	ImGui::End();
}