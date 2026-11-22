#pragma once
#include <string>
#include <vector>

#include "StudioConditionTarget.h"
#include "Engine/Data/StateMachineData.h"

// 写す元の候補1件
struct ConditionCopyItem
{
	ConditionTarget target; // どのリンクか
	std::string group;      // 見出し(子のときはState名、親のときは空)
	std::string label;      // 表示名
};

// 設定を写す依頼
struct ConditionCopyRequest
{
	ConditionTarget source; // 写す元
	ConditionTarget target; // 写す先

	bool IsValid() const { return source.IsValid() && target.IsValid(); }
};

// 同じ階層のリンクから、設定を写すためのリスト
namespace ConditionCopyList
{
	// ドラッグで運ぶ物の種類
	inline constexpr const char* kPayloadType = "CONDITION_LINK";

	// 親の階層のリンクを集める
	std::vector<ConditionCopyItem> CollectParent(const StateMachineData& data, const ConditionTarget& self);
	// 子の階層のリンクを集める
	std::vector<ConditionCopyItem> CollectChild(const StateMachineData& data, const ConditionTarget& self);

	// リストを描く
	void Draw(const std::vector<ConditionCopyItem>& items);

	// 受け口を描く
	ConditionTarget DrawDropTarget(const ConditionTarget& self);
}