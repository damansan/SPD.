#pragma once
#include "entt.hpp"
#include "Engine/Data/ConditionGraphData.h"

class ConditionGraphEvaluator
{
public:
	// 遷移条件が成立していればtrueを返す
	static bool Evaluate(const ConditionGraph& graph, entt::registry& registry, entt::entity entity);

private:
	// ゲッターノードで取得している値を取得する
	static float GetGetterValue(const ConditionGetterNode& node, entt::registry& registry, entt::entity entity);
	// 変数ノードで保持している値を取得する
	static float GetVariableValue(const ConditionVariableNode& node);

	// 比較ノードを演算し結果を返す
	static bool EvaluateOperator(const ConditionGraph& graph, const ConditionOperatorNode& op, entt::registry& registry, entt::entity entity);

	// 論理ノードを演算し結果を返す
	static bool EvaluateLogic(const ConditionGraph& graph, const ConditionLogicNode& logic, entt::registry& registry, entt::entity entity);

	// 否定ノードを演算し結果を返す
	static bool EvaluateNot(const ConditionGraph& graph, const ConditionNotNode& notNode, entt::registry& registry, entt::entity entity);

	// ノードIDから入力値をfloatとして解決する
	static float ResolveInputValue(const ConditionGraph& graph, int nodeId, entt::registry& registry, entt::entity entity);
};