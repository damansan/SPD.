#pragma once
#include "entt.hpp"
#include "Engine/Data/SequencerData.h"
#include "Engine/Core/JsonFileCache.h"
#include <string>
#include <unordered_map>

class SequencerSystem
{
public:
	void Update(entt::registry& registry);

private:
	JsonFileCache<SequencerData> dataCache;
	
	// 今回のフレームで再生した区間
	struct PlayedSpan
	{
		float current = 0.0f;   // 今の時刻
		float from = 0.0f;      // 進む前
		float to = 0.0f;        // 進んだ後
		bool  wrapped = false;  // ループで頭に戻ったか
		float clipStart = 0.0f; // クリップの頭
		float clipEnd = 0.0f;   // クリップの最後
	};

	// 1体のエンティティについて、フラグを更新する
	void UpdateFlags(SequencerComponent& seq, const SequencerClipData* clip, const PlayedSpan& span);
};