#pragma once
#include <string>
#include <vector>
#include <json.hpp>

// ルートモーションの移動量を変える区間
struct AnimationEditSection
{
	float start = 0.0f;
	float end = 0.0f;
	float scale = 1.0f; // 移動量の倍率
};

// 1クリップ分の編集設定
struct AnimationEditClipData
{
	std::string clipName;
	float yawOffsetDegrees = 0.0f; // 向きの補正

	// トリミング
	float trimStart = 0.0f;
	float trimEnd = -1.0f;

	// ルートモーションの移動量を変える区間
	std::vector<AnimationEditSection> sectionsXZ; // 水平移動
	std::vector<AnimationEditSection> sectionsY;  // 上下移動
};

// モデル1体分、全クリップの編集設定をまとめたもの
class AnimationEditData
{
public:
	std::string rootNodeName; // 焼き込み対象のルートノード名
	std::vector<AnimationEditClipData> clips;

	// 指定したクリップ名のデータを探す
	const AnimationEditClipData* FindClip(const std::string& clipName) const;

	// 指定したクリップ名のデータを探す
	AnimationEditClipData* FindOrCreateClip(const std::string& clipName);

	// 保存/読み込み
	nlohmann::ordered_json ToJson() const;
	void FromJson(const nlohmann::json& j);

	// モデルパスから保存先のパスを組み立てる
	static std::string GetFilePath(const std::string& modelPath);
};