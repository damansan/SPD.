#pragma once

// 条件ウィンドウで編集する条件がどのリンクから来たものか
struct ConditionTarget
{
	enum class Kind
	{
		None,      // 何も編集していない
		StateLink, // 親のリンク(エントリー → State ・ State → State)
		EntryLink, // 子のリンク(エントリー → クリップ)
		ClipLink,  // 子のリンク(クリップ 　→ クリップ)
	};

	Kind kind = Kind::None;
	int stateId = -1; // 子のリンクのとき：どのStateの子か
	int linkId = -1;

	bool IsValid() const { return kind != Kind::None; }
	bool operator==(const ConditionTarget& o) const { return kind == o.kind && stateId == o.stateId && linkId == o.linkId; }
	bool operator!=(const ConditionTarget& o) const { return !(*this == o); }
};