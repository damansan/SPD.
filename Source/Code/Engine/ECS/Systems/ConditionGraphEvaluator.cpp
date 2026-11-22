#include "ConditionGraphEvaluator.h"
#include "Engine/Resource/ComponentRegistry.h"
#include "Engine/ECS/Reflection.h"
#include <DirectXMath.h>
#include <cmath>

// 遷移条件が成立していればtrueを返す
bool ConditionGraphEvaluator::Evaluate(const ConditionGraph& graph, entt::registry& registry, entt::entity entity)
{
	int fromId = -1;
	for (const auto& link : graph.links)
	{
		if (link.toId == graph.returnNode.id)
		{
			fromId = link.fromId;
			break;
		}
	}
	if (fromId == -1) return false; // Returnに何もつながっていないなら不成立

	// 繋がっているのがOperatorなら、その比較結果をそのまま返す
	for (const auto& o : graph.operatorNodes)
		if (o.id == fromId) return EvaluateOperator(graph, o, registry, entity);
	// 繋がっているのがLogic(AND/OR)なら、その結果をそのまま返す
	for (const auto& l : graph.logicNodes)
		if (l.id == fromId) return EvaluateLogic(graph, l, registry, entity);
	// 繋がっているのがGetter(Bool型)なら、0.5より上でtrueとみなす
	for (const auto& g : graph.getterNodes)
		if (g.id == fromId) return GetGetterValue(g, registry, entity) > 0.5f;
	// 繋がっているのがVariable(Bool型)なら、0.5より上でtrueとみなす
	for (const auto& v : graph.variableNodes)
		if (v.id == fromId) return GetVariableValue(v) > 0.5f;
	// 繋がっているのがNotなら、その結果をそのまま返す
	for (const auto& n : graph.notNodes)
		if (n.id == fromId) return EvaluateNot(graph, n, registry, entity);

	return false; // どれにも該当しない場合
}

// ゲッターノードで取得している値を取得する
float ConditionGraphEvaluator::GetGetterValue(const ConditionGetterNode& node, entt::registry& registry, entt::entity entity)
{
	// コンポーネント名とフィールドのコード上の名前から、FieldEntryを探す
	const FieldEntry* found = Reflection::FindField(node.componentName, node.fieldName);
	if (!found) return 0.0f;

	// コンポーネントの実体を取得する
	entt::meta_any instance;

	// ComponentRegistryに登録されていれば、そちらから取得する
	if (const ComponentInfo* info = ComponentRegistry::Get(node.componentName)) 
	{
		instance = info->getMeta(registry, entity);
	}
	else
	{
		// 登録されていない(他のComponentとまとめて付与される等)場合は、
		// 型情報からEnTTのストレージを直接引いて取得する
		if (auto* storage = registry.storage(found->componentType.info().hash()))
		{
			if (storage->contains(entity))
			{
				instance = found->componentType.from_void(storage->value(entity));
			}
		}
	}

	if (!instance) return 0.0f;

	// 型情報からフィードの値を取得する
	entt::meta_data data = found->componentType.data(found->fieldId);
	entt::meta_any value = data.get(instance);

	// ネストしたフィールドの場合、もう1段階だけ掘り下げる
	if (found->subFieldId != 0 && found->nestedType)
	{
		entt::meta_data subData = found->nestedType.data(found->subFieldId);
		value = subData.get(value);
	}

	// 各型をfloatに変換する
	if (float* f = value.try_cast<float>()) return *f;
	if (int* i = value.try_cast<int>())     return static_cast<float>(*i);
	if (bool* b = value.try_cast<bool>())   return *b ? 1.0f : 0.0f;

	if (DirectX::XMFLOAT2* v = value.try_cast<DirectX::XMFLOAT2>())
	{
		switch (node.selectedComponent)
		{
		case FloatComponent::X: return v->x;
		case FloatComponent::Y: return v->y;
		default: return 0.0f;
		}
	}
	if (DirectX::XMFLOAT3* v = value.try_cast<DirectX::XMFLOAT3>())
	{
		switch (node.selectedComponent)
		{
		case FloatComponent::X: return v->x;
		case FloatComponent::Y: return v->y;
		case FloatComponent::Z: return v->z;
		default: return 0.0f;
		}
	}
	if (DirectX::XMFLOAT4* v = value.try_cast<DirectX::XMFLOAT4>())
	{
		switch (node.selectedComponent)
		{
		case FloatComponent::X: return v->x;
		case FloatComponent::Y: return v->y;
		case FloatComponent::Z: return v->z;
		case FloatComponent::W: return v->w;
		default: return 0.0f;
		}
	}

	return 0.0f;
}
// 変数ノードで保持している値を取得する
float ConditionGraphEvaluator::GetVariableValue(const ConditionVariableNode& node)
{
	switch (node.type)
	{
	case ValueType::Int:   return static_cast<float>(node.intValue);
	case ValueType::Float: return node.floatValue;
	case ValueType::Bool:  return node.boolValue ? 1.0f : 0.0f;
	default:               return 0.0f;
	}
}

// 比較ノードを演算し結果を返す
bool ConditionGraphEvaluator::EvaluateOperator(const ConditionGraph& graph, const ConditionOperatorNode& op, entt::registry& registry, entt::entity entity)
{
	// A/Bそれぞれに繋がっているノードのIDを探す
	int fromIdA = -1;
	int fromIdB = -1;
	for (const auto& link : graph.links)
	{
		if (link.toId != op.id) continue;
		if (link.toPinB) fromIdB = link.fromId;
		else             fromIdA = link.fromId;
	}

	if (fromIdA == -1 || fromIdB == -1) return false; // 片方でも未接続なら、不成立扱い

	float a = ResolveInputValue(graph, fromIdA, registry, entity);
	float b = ResolveInputValue(graph, fromIdB, registry, entity);

	const float epsilon = 0.0001f; // 浮動小数点の誤差を吸収するための許容範囲

	switch (op.op)
	{
	case CompareOp::Greater:      return a > b;
	case CompareOp::Less:         return a < b;
	case CompareOp::Equal:        return std::abs(a - b) < epsilon;
	case CompareOp::NotEqual:     return std::abs(a - b) >= epsilon;
	case CompareOp::GreaterEqual: return a >= b;
	case CompareOp::LessEqual:    return a <= b;
	}
	return false;
}

bool ConditionGraphEvaluator::EvaluateLogic(const ConditionGraph& graph, const ConditionLogicNode& logic, entt::registry& registry, entt::entity entity)
{
	// A/Bそれぞれに繋がっているノードIDを探す
	int fromIdA = -1;
	int fromIdB = -1;
	for (const auto& link : graph.links)
	{
		if (link.toId != logic.id) continue;
		if (link.toPinB) fromIdB = link.fromId;
		else             fromIdA = link.fromId;
	}

	if (fromIdA == -1 || fromIdB == -1) return false; // 片方でも未接続なら、不成立

	bool a = ResolveInputValue(graph, fromIdA, registry, entity) > 0.5f;
	bool b = ResolveInputValue(graph, fromIdB, registry, entity) > 0.5f;

	switch (logic.op)
	{
	case LogicOp::And: return a && b;
	case LogicOp::Or:  return a || b;
	}
	return false;
}

// 否定ノードを演算し結果を返す
bool ConditionGraphEvaluator::EvaluateNot(const ConditionGraph& graph, const ConditionNotNode& notNode, entt::registry& registry, entt::entity entity)
{
	// 入力ピン(1本のみ)に繋がっているノードIDを探す
	int fromId = -1;
	for (const auto& link : graph.links)
	{
		if (link.toId == notNode.id)
		{
			fromId = link.fromId;
			break;
		}
	}

	if (fromId == -1) return false; // 未接続なら不成立扱い

	bool input = ResolveInputValue(graph, fromId, registry, entity) > 0.5f;
	return !input;
}

// ノードIDから入力値をfloatとして解決する
float ConditionGraphEvaluator::ResolveInputValue(const ConditionGraph& graph, int nodeId, entt::registry& registry, entt::entity entity)
{
	for (const auto& g : graph.getterNodes)
		if (g.id == nodeId) return GetGetterValue(g, registry, entity);

	for (const auto& v : graph.variableNodes)
		if (v.id == nodeId) return GetVariableValue(v);

	for (const auto& o : graph.operatorNodes)
		if (o.id == nodeId) return EvaluateOperator(graph, o, registry, entity) ? 1.0f : 0.0f;

	for (const auto& l : graph.logicNodes)
		if (l.id == nodeId) return EvaluateLogic(graph, l, registry, entity) ? 1.0f : 0.0f;

	for (const auto& n : graph.notNodes)
		if (n.id == nodeId) return EvaluateNot(graph, n, registry, entity) ? 1.0f : 0.0f;


	return 0.0f; // どのノードにも該当しない場合
}
