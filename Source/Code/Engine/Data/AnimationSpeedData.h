#pragma once
#include <string>
#include <vector>
#include <json.hpp>

// 速度ポイント
struct AnimationSpeedPoint
{
	int id = -1;
	float time = 0.0f;
	float speed = 1.0f;
	bool  isEndpoint = false;
};

// 1クリップ分の速度ポイント群
struct AnimationSpeedClipData
{
	std::string clipName;
	std::vector<AnimationSpeedPoint> points;

	int nextId = 1;
	int GenerateId() { return nextId++; }

	// 正規化時間における速度を線形補完で求める
	float EvaluateSpeed(float normalizedTime) const;
};

// モデル1体分、全クリップの速度データをまとめたもの
class AnimationSpeedData
{
public:
	std::vector<AnimationSpeedClipData> clips;

	// 指定したクリップ名のデータを探す
	AnimationSpeedClipData* FindOrCreateClip(const std::string& clipName);

	// 保存/読み込み
	nlohmann::ordered_json ToJson() const;
	void FromJson(const nlohmann::json& j);
};