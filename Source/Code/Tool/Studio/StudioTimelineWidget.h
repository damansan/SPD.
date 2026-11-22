#pragma once
#include <imgui.h>
#include <vector>

struct AnimatorComponent;
struct StudioContext;

class StudioTimelineWidget
{
public:
	// 目盛りを描いて、ピンのドラッグを処理する
	bool Begin(const char* id, float clipLength, float frameStep, float& inOutTime);
	
	// ドラッグしたら animator を止めてポーズだけ更新する
	bool BeginWithAnimator(const char* id, float clipLength, float frameStep, AnimatorComponent& animator, StudioContext& context);
	// animator を再生するボタン
	static void DrawAnimatorToolbar(AnimatorComponent& animator, float clipLength, float frameStep);

	// トラックを描き終えたら呼ぶ
	void End();

	// 再生ボタンと現在時刻
	static bool DrawToolbar(bool& isPlaying, float time, float frameStep);
	// フレームに吸い付かせる
	static float SnapToFrame(float t, float frameStep, float clipLength);

	// 範囲の最低の長さ
	static float MinLength(float frameStep) { return (frameStep > 0.0f) ? frameStep : 0.001f; }
	// 時刻からフレームの番号
	static int ToFrame(float t, float frameStep);

	// タイムライン上の範囲
	struct TimeSpan
	{
		float start = 0.0f;
		float end = 0.0f;
	};

	// 重ならない範囲の並びで、index 番目が動ける所
	static void GetSpanLimits(const std::vector<TimeSpan>& spans, int index, float clipLength, float& outLower, float& outUpper);
	// pinTime に新しい範囲を追加できるか
	static bool FindAddEnd(const std::vector<TimeSpan>& spans, float pinTime, float clipLength, float minLength, float& outEnd);

	// 範囲の端のピン
	bool DrawRangeHandles(const char* id, float handleY, float frameStep, float minLength, float& inOutStart, float& inOutEnd,
		float lowerLimit = 0.0f, float upperLimit = -1.0f, float lineTopY = -1.0f);

	// ピンの向き
	enum class PinDirection
	{
		Down, // 頭が上、とがった先が下
		Up,   // 頭が下、とがった先が上
	};

	// ピンを1本描く
	static void DrawPin(ImDrawList* drawList, float x, float tipY, float headHeight, float radius, ImU32 color, PinDirection direction);

	static constexpr float RangeHandleHeight = 16.0f; // 範囲の端のピンの頭の高さ

	// 時刻とX座標の変換
	float TimeToX(float t) const;
	float XToTime(float x) const;

	// トラックを描ける左右の端
	float GetLeft() const { return left; }
	float GetRight() const { return left + width; }

private:
	static constexpr float PinHeadHeight = 16.0f; // 目盛りの上に、ピンの頭を置く余白
	static constexpr float RulerHeight = 24.0f;   // 目盛りの帯の高さ
	static constexpr float LeftPadding = 12.0f;   // 左の余白
	static constexpr float RightPadding = 28.0f;  // 右の余白

	float left = 0.0f;   // 0秒の位置
	float width = 1.0f;  // 0秒から最後までの幅
	float top = 0.0f;    // 目盛りの上端
	float length = 1.0f; // クリップの長さ
	float time = 0.0f;   // ピンの時刻

	// 目盛り
	void DrawRuler(ImDrawList* drawList, float frameStep) const;
	// ピンの頭
	void DrawPinHead(ImDrawList* drawList) const;
	void DrawRangeHandle(ImDrawList* drawList, float x, float handleY, float lineTopY, bool highlighted) const;

	// 範囲の端のピンをつかめる幅の半分
	static constexpr float RangeHandleHalfWidth = 7.0f;
};