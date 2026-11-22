#pragma once
#include <deque>
#include <string>
#include <vector>
#include "entt.hpp"

// Player の移動の様子
class MovementMonitorWindow
{
public:
	// 値を1つ記録する
	void Record(entt::registry& registry, float elapsedTime);

	// 記録をグラフに描く
	void Draw();

	// 表示しているか
	bool isOpen = false;

private:
	// 1回分の記録
	struct Sample
	{
		float time = 0.0f;            // 記録を始めてからの時間
		float horizontalSpeed = 0.0f; // 水平の速さ
		float desiredSpeed = 0.0f;    // 目標の速さ
		float verticalSpeed = 0.0f;   // 縦の速さ
		int   stateIndex = 0;         // ステートの番号
	};

	// グラフに描く1本の線
	struct Series
	{
		float Sample::* value = nullptr; // どの値を描くか
		unsigned int    color = 0;
		float           thickness = 1.0f;
	};

	// グラフを1つ描く
	void DrawGraph(float height, float minValue, float maxValue, float gridStep, const Series* series, int seriesCount);

	// ステート名を番号に変える
	int FindOrAddState(const std::string& name);

	std::deque<Sample>       samples;    // 古い順
	std::vector<std::string> stateNames; // ステート名

	float totalTime = 0.0f;   // 記録を始めてからの時間
	bool  paused = false;     // 一時停止中は記録しない

	static constexpr float kViewSeconds = 3.0f;  // 表示する時間の幅
	static constexpr float kKeepSeconds = 10.0f; // 記録として残しておく長さ
};