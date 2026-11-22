#pragma once
#include <imgui.h>

// Stateグラフで使用する共通処理
namespace StateGraphDraw
{
	// 矩形の中心からtargetへ伸ばした線が、矩形の縁と交わる点
	ImVec2 GetEdgePoint(const ImVec2& rectMin, const ImVec2& rectMax, const ImVec2& target);

	// 2つのノードを結ぶリンクの始点と終点を求める
	void CalcLinkPoints(
		const ImVec2& fromMin, const ImVec2& fromMax, int fromKey,
		const ImVec2& toMin, const ImVec2& toMax, int toKey,
		bool hasReverse, int lane,
		ImVec2& outStart, ImVec2& outEnd);

	// 矢印描画
	void DrawArrow(ImDrawList* drawList, const ImVec2& start, const ImVec2& end, ImU32 color, float thickness);

	// 点と線分の距離(リンクのクリック判定に使う)
	float DistanceToSegment(const ImVec2& p, const ImVec2& a, const ImVec2& b);

	// 削除ボタン
	bool DeleteButton(const char* label);
}