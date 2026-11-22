#pragma once
#include <set>
#include <string>
#include <vector>
#include "entt.hpp"
#include "Engine/Core/JsonFileCache.h"
#include "Engine/Data/StateMachineData.h"

struct AnimatorComponent;
struct StateMachineComponent;

// 読み込んだグラフと、その結果
struct LoadedStateMachine
{
	StateMachineData data;
	bool loaded = false; // JSONとして読み込めたか
	bool valid = false;  // 検証を通ったか
};

// Stateマシン(Stateの遷移と、アニメーションの切り替え)
class StateMachineSystem
{
public:
	// Stateの遷移を決める (ゲームプレイの各Systemより前に呼ぶ)
	void UpdateState(entt::registry& registry);

	// 再生するクリップを決める (ゲームプレイの各Systemより前に呼ぶ)
	void UpdateAnimation(entt::registry& registry);

	// 使用するState名を申告する(グラフの読み込み時に、存在するかを確かめる)
	template<typename UserComponent>
	static void RequireState(const std::string& stateName, const std::string& user)
	{
		AddRequiredState(stateName, user,
			[](const entt::registry& registry, entt::entity entity) { return registry.all_of<UserComponent>(entity); });
	}

private:
	
	// エンティティが、申告したSystemの対象か(UserComponentを持っているか)
	using UsesStateFunc = bool(*)(const entt::registry& registry, entt::entity entity);

	// 申告されたState名と、申告した側の名前
	struct RequiredState
	{
		std::string stateName;
		std::string user;
		UsesStateFunc usesState = nullptr; // 申告したSystemの対象か
	};
	static std::vector<RequiredState>& GetRequiredStates();
	// 申告を一覧に足す
	static void AddRequiredState(const std::string& stateName, const std::string& user, UsesStateFunc usesState);

	// 申告されたState名が、このエンティティのグラフにあるかを確かめる
	void CheckRequiredStates(const StateMachineData& data, const std::string& path, const entt::registry& registry, entt::entity entity);


	// グラフを取得する (読み込めない・検証エラーの場合はnullptr)
	const StateMachineData* Load(const std::string& path);

	// 条件が成立した最初の遷移を返す(優先度順)
	static const StateTransition* FindPassedTransition(
		const std::vector<StateTransition>& transitions, entt::registry& registry, entt::entity entity);

	// Stateに入る
	static void EnterState(StateMachineComponent& sm, const StateNode& state, const StateTransition& via);

	// クリップを再生する
	static void PlayClip(AnimatorComponent& animator, const ClipNode& clip, const BlendSetting& blend);

	JsonFileCache<LoadedStateMachine> graphCache;

	// 使えないことをLogに報告済みのパス
	std::set<std::string> reportedPaths;
	// 「Stateが無い」と報告済みの組
	std::set<std::string> reportedMissingStates;
};