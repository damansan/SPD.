#include "StudioStateGraphDraw.h"
#include <cmath>
#include <cfloat>

namespace StateGraphDraw
{
	// 矩形の中心からtargetへ伸ばした線が、矩形の縁と交わる点
	ImVec2 GetEdgePoint(const ImVec2& rectMin, const ImVec2& rectMax, const ImVec2& target)
	{
		ImVec2 center((rectMin.x + rectMax.x) * 0.5f, (rectMin.y + rectMax.y) * 0.5f);
		float halfW = (rectMax.x - rectMin.x) * 0.5f;
		float halfH = (rectMax.y - rectMin.y) * 0.5f;

		ImVec2 dir(target.x - center.x, target.y - center.y);
		if (dir.x == 0.0f && dir.y == 0.0f) return center;

		// 横の辺と縦の辺のうち、先にぶつかるほうの倍率を使う
		float scaleX = (dir.x != 0.0f) ? halfW / fabsf(dir.x) : FLT_MAX;
		float scaleY = (dir.y != 0.0f) ? halfH / fabsf(dir.y) : FLT_MAX;
		float scale = (scaleX < scaleY) ? scaleX : scaleY;
		return ImVec2(center.x + dir.x * scale, center.y + dir.y * scale);
	}

	// 2つのノードを結ぶリンクの始点と終点を求める
	void CalcLinkPoints(
		const ImVec2& fromMin, const ImVec2& fromMax, int fromKey,
		const ImVec2& toMin, const ImVec2& toMax, int toKey,
		bool hasReverse, int lane,
		ImVec2& outStart, ImVec2& outEnd)
	{
		ImVec2 fromCenter((fromMin.x + fromMax.x) * 0.5f, (fromMin.y + fromMax.y) * 0.5f);
		ImVec2 toCenter((toMin.x + toMax.x) * 0.5f, (toMin.y + toMax.y) * 0.5f);

		// ノードの中心同士を結ぶ線が、それぞれの縁と交わる点を、始点と終点にする
		outStart = GetEdgePoint(fromMin, fromMax, toCenter);
		outEnd = GetEdgePoint(toMin, toMax, fromCenter);

		float offset = (hasReverse ? 8.0f : 0.0f) + lane * 10.0f;
		if (offset <= 0.0f) return;

		// 往復の線が反対側にずれるように、キーの小さいほうから見た向きを基準にする
		bool fromIsSmaller = fromKey < toKey;
		ImVec2 baseFrom = fromIsSmaller ? fromCenter : toCenter;
		ImVec2 baseTo = fromIsSmaller ? toCenter : fromCenter;
		ImVec2 dir(baseTo.x - baseFrom.x, baseTo.y - baseFrom.y);
		float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
		if (len < 0.0001f) return;

		// 基準の向きに垂直な方向へずらす(逆向きの線は反対側へ)
		float sign = (hasReverse && !fromIsSmaller) ? -1.0f : 1.0f;
		ImVec2 perp(-dir.y / len * offset * sign, dir.x / len * offset * sign);
		outStart = ImVec2(outStart.x + perp.x, outStart.y + perp.y);
		outEnd = ImVec2(outEnd.x + perp.x, outEnd.y + perp.y);
	}

	// 矢印描画
	void DrawArrow(ImDrawList* drawList, const ImVec2& start, const ImVec2& end, ImU32 color, float thickness)
	{
		ImVec2 dir(end.x - start.x, end.y - start.y);
		float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
		if (len < 0.0001f) return;
		dir.x /= len;
		dir.y /= len;

		const float arrowLength = 14.0f;
		const float arrowWidth = 11.0f;

		// 線は矢印の根元まで
		ImVec2 base(end.x - dir.x * arrowLength, end.y - dir.y * arrowLength);
		drawList->AddLine(start, base, color, thickness);

		// 矢印の三角形
		ImVec2 perp(-dir.y, dir.x);
		ImVec2 left(base.x + perp.x * arrowWidth * 0.5f, base.y + perp.y * arrowWidth * 0.5f);
		ImVec2 right(base.x - perp.x * arrowWidth * 0.5f, base.y - perp.y * arrowWidth * 0.5f);
		drawList->AddTriangleFilled(end, left, right, color);
	}

	// 点と線分の距離
	float DistanceToSegment(const ImVec2& p, const ImVec2& a, const ImVec2& b)
	{
		ImVec2 ab(b.x - a.x, b.y - a.y);
		ImVec2 ap(p.x - a.x, p.y - a.y);
		float lenSq = ab.x * ab.x + ab.y * ab.y;

		// 点pを線分abに投影した位置(0がa、1がb)。線分の外に出たら端に寄せる
		float t = (lenSq > 0.0f) ? (ap.x * ab.x + ap.y * ab.y) / lenSq : 0.0f;
		t = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);

		// 線分上で一番近い点との距離
		ImVec2 closest(a.x + ab.x * t, a.y + ab.y * t);
		float dx = p.x - closest.x;
		float dy = p.y - closest.y;
		return sqrtf(dx * dx + dy * dy);
	}

	// 削除ボタン
	bool DeleteButton(const char* label)
	{
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
		bool pressed = ImGui::Button(label, ImVec2(-1.0f, 0.0f));
		ImGui::PopStyleColor(3);

		return pressed;
	}
}