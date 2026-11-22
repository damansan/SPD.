#pragma once
#include <string>
#include <vector>
#include <json.hpp>
#include "Engine/ECS/Components/SequencerComponent.h"


// 範囲 : 開始から終了の間常にtrueを返す
struct SequencerRange
{
	int id = -1;
	float start = 0.0f; // 開始時刻
	float end = 0.0f;   // 終了時刻
	SequencerFlag flag = SequencerFlag::AnimExit;
};

// 瞬間 : 通過したそのフレームだけtrue
struct SequencerPoint
{
	int id = -1;
	float time = 0.0f; // 時刻
	SequencerFlag flag = SequencerFlag::AnimExit;
};

// 1クリップ分のRange/Pointをまとめたもの
struct SequencerClipData
{
	std::string clipName;
	std::vector<SequencerRange> ranges;
	std::vector<SequencerPoint> points;

	int nextId = 1;
	int GenerateId() { return nextId++; }
};

// モデル1体分、全クリップのSequencer設定をまとめたもの
class SequencerData
{
public:
	std::vector<SequencerClipData> clips;

	// 指定したクリップ名のデータを探す
	const SequencerClipData* FindClip(const std::string& clipName) const;
	SequencerClipData* FindClip(const std::string& clipName);
	SequencerClipData* FindOrCreateClip(const std::string& clipName);

	// 保存/読み込み
	nlohmann::ordered_json ToJson() const;
	void FromJson(const nlohmann::json& j);

	// モデルパスから保存先のパスを組み立てる
	static std::string GetFilePath(const std::string& modelPath);
};