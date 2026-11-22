#include "ConditionGraphData.h"
#include <algorithm>

namespace
{
	// ValueType
	std::string ToString(ValueType type)
	{
		switch (type)
		{
		case ValueType::Int:         return "Int";
		case ValueType::Float:       return "Float";
		case ValueType::Bool:        return "Bool";
		case ValueType::Float2:      return "Float2";
		case ValueType::Float3:      return "Float3";
		case ValueType::Float4:      return "Float4";
		case ValueType::Unsupported: return "Unsupported";
		default:                     return "Float";
		}
	}
	ValueType ToValueType(const std::string& s)
	{
		if (s == "Int")         return ValueType::Int;
		if (s == "Bool")        return ValueType::Bool;
		if (s == "Float2")      return ValueType::Float2;
		if (s == "Float3")      return ValueType::Float3;
		if (s == "Float4")      return ValueType::Float4;
		if (s == "Unsupported") return ValueType::Unsupported;
		return ValueType::Float;
	}

	// FloatComponent
	std::string ToString(FloatComponent c)
	{
		switch (c)
		{
		case FloatComponent::X: return "X";
		case FloatComponent::Y: return "Y";
		case FloatComponent::Z: return "Z";
		case FloatComponent::W: return "W";
		default:                return "None";
		}
	}
	FloatComponent ToFloatComponent(const std::string& s)
	{
		if (s == "X") return FloatComponent::X;
		if (s == "Y") return FloatComponent::Y;
		if (s == "Z") return FloatComponent::Z;
		if (s == "W") return FloatComponent::W;
		return FloatComponent::None;
	}

	// CompareOp
	std::string ToString(CompareOp op)
	{
		switch (op)
		{
		case CompareOp::Greater:      return "Greater";
		case CompareOp::Less:         return "Less";
		case CompareOp::Equal:        return "Equal";
		case CompareOp::NotEqual:     return "NotEqual";
		case CompareOp::GreaterEqual: return "GreaterEqual";
		case CompareOp::LessEqual:    return "LessEqual";
		default:                      return "Greater";
		}
	}
	CompareOp ToCompareOp(const std::string& s)
	{
		if (s == "Less")         return CompareOp::Less;
		if (s == "Equal")        return CompareOp::Equal;
		if (s == "NotEqual")     return CompareOp::NotEqual;
		if (s == "GreaterEqual") return CompareOp::GreaterEqual;
		if (s == "LessEqual")    return CompareOp::LessEqual;
		return CompareOp::Greater;
	}

	// LogicOp
	std::string ToString(LogicOp op)
	{
		switch (op)
		{
		case LogicOp::And: return "And";
		case LogicOp::Or:  return "Or";
		default:           return "And";
		}
	}
	LogicOp ToLogicOp(const std::string& s)
	{
		if (s == "Or") return LogicOp::Or;
		return LogicOp::And;
	}

	// IDで要素を探す
	template<typename T>
	const T* FindById(const std::vector<T>& list, int id)
	{
		for (const auto& e : list)
		{
			if (e.id == id) return &e;
		}
		return nullptr;
	}

	// IDで要素を消す
	template<typename T>
	bool EraseById(std::vector<T>& list, int id)
	{
		auto it = std::remove_if(list.begin(), list.end(), [id](const T& e) { return e.id == id; });
		if (it == list.end()) return false;
		list.erase(it, list.end());
		return true;
	}

	// 大小の比較か
	bool IsOrderCompareOp(CompareOp op)
	{
		return op == CompareOp::Greater || op == CompareOp::Less
			|| op == CompareOp::GreaterEqual || op == CompareOp::LessEqual;
	}
}

// 使用中のIDの最大値+1に、採番をそろえる
void ConditionGraph::RecalculateNextId()
{
	int maxId = returnNode.id;
	for (const auto& n : variableNodes) maxId = (std::max)(maxId, n.id);
	for (const auto& n : getterNodes)   maxId = (std::max)(maxId, n.id);
	for (const auto& n : operatorNodes) maxId = (std::max)(maxId, n.id);
	for (const auto& n : logicNodes)    maxId = (std::max)(maxId, n.id);
	for (const auto& n : notNodes)      maxId = (std::max)(maxId, n.id);
	for (const auto& l : links)         maxId = (std::max)(maxId, l.id);
	nextId = maxId + 1;
}

// Returnに何もつながっていないか
bool ConditionGraph::IsEmpty() const
{
	for (const auto& l : links)
	{
		if (l.toId == returnNode.id) return false;
	}
	return true;
}

// ノードの種類
ConditionNodeKind ConditionGraph::GetNodeKind(int nodeId) const
{
	if (nodeId == returnNode.id)            return ConditionNodeKind::Return;
	if (FindById(variableNodes, nodeId))    return ConditionNodeKind::Variable;
	if (FindById(getterNodes, nodeId))      return ConditionNodeKind::Getter;
	if (FindById(operatorNodes, nodeId))    return ConditionNodeKind::Operator;
	if (FindById(logicNodes, nodeId))       return ConditionNodeKind::Logic;
	if (FindById(notNodes, nodeId))         return ConditionNodeKind::Not;
	return ConditionNodeKind::None;
}

// 出力ピンが出す値の型
bool ConditionGraph::GetOutputType(int nodeId, ValueType& outType) const
{
	switch (GetNodeKind(nodeId))
	{
	case ConditionNodeKind::Variable:
		outType = FindById(variableNodes, nodeId)->type;
		return true;

	case ConditionNodeKind::Getter:
	{
		const ConditionGetterNode* g = FindById(getterNodes, nodeId);
		// Float2～4は、使える成分(.xyz)を選んでいればFloatとして扱う
		outType = IsGetterAxisValid(*g) ? ValueType::Float : GetGetterFieldType(*g);
		return true;
	}
	// 比較・論理・否定の出力は常にBool
	case ConditionNodeKind::Operator:
	case ConditionNodeKind::Logic:
	case ConditionNodeKind::Not:
		outType = ValueType::Bool;
		return true;

		// Returnは出力ピンを持たない
	default:
		return false;
	}
}

// 変数ノードの追加
int ConditionGraph::AddVariableNode(ValueType type, float x, float y)
{
	ConditionVariableNode n;
	n.id = GenerateId();
	n.posX = x;
	n.posY = y;
	n.type = type;
	variableNodes.push_back(n);
	return n.id;
}
// ゲッターノードの追加
int ConditionGraph::AddGetterNode(float x, float y)
{
	ConditionGetterNode n;
	n.id = GenerateId();
	n.posX = x;
	n.posY = y;
	getterNodes.push_back(n);
	return n.id;
}
// 比較ノード追加
int ConditionGraph::AddOperatorNode(CompareOp op, float x, float y)
{
	ConditionOperatorNode n;
	n.id = GenerateId();
	n.posX = x;
	n.posY = y;
	n.op = op;
	operatorNodes.push_back(n);
	return n.id;
}
// 論理演算ノード追加
int ConditionGraph::AddLogicNode(LogicOp op, float x, float y)
{
	ConditionLogicNode n;
	n.id = GenerateId();
	n.posX = x;
	n.posY = y;
	n.op = op;
	logicNodes.push_back(n);
	return n.id;
}
// 否定ノード追加
int ConditionGraph::AddNotNode(float x, float y)
{
	ConditionNotNode n;
	n.id = GenerateId();
	n.posX = x;
	n.posY = y;
	notNodes.push_back(n);
	return n.id;
}

// ノードの削除
void ConditionGraph::RemoveNode(int nodeId)
{
	// Returnは条件グラフに必ず1つあるので消さない
	if (nodeId == returnNode.id) return;

	bool removed = EraseById(variableNodes, nodeId)
		|| EraseById(getterNodes, nodeId)
		|| EraseById(operatorNodes, nodeId)
		|| EraseById(logicNodes, nodeId)
		|| EraseById(notNodes, nodeId);
	if (!removed) return;

	// つながっていたリンクも消す
	links.erase(
		std::remove_if(links.begin(), links.end(),
			[nodeId](const ConditionLink& l) { return l.fromId == nodeId || l.toId == nodeId; }),
		links.end());
}

// リンクを作れるか
bool ConditionGraph::CanLink(int fromNodeId, int toNodeId, bool toPinB, std::string* reason) const
{
	return CheckLink(fromNodeId, toNodeId, toPinB, -1, reason);
}
// リンクを作る
int ConditionGraph::AddLink(int fromNodeId, int toNodeId, bool toPinB)
{
	if (!CanLink(fromNodeId, toNodeId, toPinB)) return -1;

	// 入力ピンには1本だけ
	links.erase(
		std::remove_if(links.begin(), links.end(),
			[=](const ConditionLink& l) { return l.toId == toNodeId && l.toPinB == toPinB; }),
		links.end());

	ConditionLink l;
	l.id = GenerateId();
	l.fromId = fromNodeId;
	l.toId = toNodeId;
	l.toPinB = toPinB;
	links.push_back(l);
	return l.id;
}

// リンクの削除
void ConditionGraph::RemoveLink(int linkId)
{
	EraseById(links, linkId);
}

// 今の設定で、リンクが決まりに合っているか
bool ConditionGraph::IsLinkValid(const ConditionLink& link, std::string* reason) const
{
	return CheckLink(link.fromId, link.toId, link.toPinB, link.id, reason);
}


// リンクの決まりの確認
bool ConditionGraph::CheckLink(int fromNodeId, int toNodeId, bool toPinB, int ignoreLinkId, std::string* reason) const
{
	auto fail = [reason](const char* message)
		{
			if (reason) *reason = message;
			return false;
		};

	if (fromNodeId == toNodeId) return fail("Cannot link a node to itself.");

	// 出す側の型
	ValueType fromType;
	if (!GetOutputType(fromNodeId, fromType)) return fail("Source has no output.");

	// ゲッターは、読めるフィールドを選んでいないとつなげない
	if (GetNodeKind(fromNodeId) == ConditionNodeKind::Getter)
	{
		const ConditionGetterNode* g = FindById(getterNodes, fromNodeId);
		if (g->componentName.empty() || g->fieldName.empty()) return fail("Select a field.");
		if (!Reflection::FindField(g->componentName, g->fieldName)) return fail("Field not found.");
	}
	if (fromType == ValueType::Unsupported) return fail("This field cannot be read in conditions.");

	// Float2～4は、成分(X/Y/Z/W)を選ばないと比べられない
	if (Reflection::GetComponentCount(fromType) > 0) return fail("Select an axis (X/Y/Z/W).");

	// 受ける側に、そのピンがあるか
	ConditionNodeKind toKind = GetNodeKind(toNodeId);
	bool hasPinA = (toKind == ConditionNodeKind::Operator || toKind == ConditionNodeKind::Logic
		|| toKind == ConditionNodeKind::Not || toKind == ConditionNodeKind::Return);
	bool hasPinB = (toKind == ConditionNodeKind::Operator || toKind == ConditionNodeKind::Logic);
	if (toPinB ? !hasPinB : !hasPinA) return fail("Target has no such input.");

	// 型の決まり
	if (toKind == ConditionNodeKind::Operator)
	{
		// Boolは == と != だけ
		const ConditionOperatorNode* o = FindById(operatorNodes, toNodeId);
		if (fromType == ValueType::Bool && IsOrderCompareOp(o->op))
		{
			return fail("Bool can only be compared with == or !=.");
		}

		// AとBは同じ型
		for (const auto& l : links)
		{
			if (l.id == ignoreLinkId) continue;
			if (l.toId != toNodeId || l.toPinB == toPinB) continue;

			ValueType otherType;
			if (GetOutputType(l.fromId, otherType) && otherType != fromType)
			{
				return fail("A and B must be the same type.");
			}
		}
	}
	else
	{
		// 論理・否定・Returnは、Boolだけ
		if (fromType != ValueType::Bool) return fail("Input must be Bool.");
	}

	// 輪を作らない
	if (DependsOn(fromNodeId, toNodeId, ignoreLinkId)) return fail("Cannot create a loop.");

	return true;
}

// nodeIdの入力を後ろへたどって、targetIdにたどり着くか
bool ConditionGraph::DependsOn(int nodeId, int targetId, int ignoreLinkId) const
{
	// たどる途中で同じノードを何度も見ないように、見たものを覚えておく
	std::vector<int> stack{ nodeId };
	std::vector<int> visited;

	while (!stack.empty())
	{
		int current = stack.back();
		stack.pop_back();
		if (current == targetId) return true;
		if (std::find(visited.begin(), visited.end(), current) != visited.end()) continue;
		visited.push_back(current);

		// currentに入ってくるリンクの元をたどる
		for (const auto& l : links)
		{
			if (l.id == ignoreLinkId) continue;
			if (l.toId == current) stack.push_back(l.fromId);
		}
	}
	return false;
}

// JSONへ変換
nlohmann::ordered_json ConditionGraphToJson(const ConditionGraph& graph)
{
	nlohmann::ordered_json jc;
	jc["returnNode"]["id"] = graph.returnNode.id;
	jc["returnNode"]["posX"] = graph.returnNode.posX;
	jc["returnNode"]["posY"] = graph.returnNode.posY;

	jc["variableNodes"] = nlohmann::json::array();
	for (const auto& v : graph.variableNodes)
	{
		nlohmann::ordered_json jv;
		jv["id"] = v.id;
		jv["posX"] = v.posX;
		jv["posY"] = v.posY;
		jv["name"] = v.name;
		jv["type"] = ToString(v.type);
		jv["intValue"] = v.intValue;
		jv["floatValue"] = v.floatValue;
		jv["boolValue"] = v.boolValue;
		jc["variableNodes"].push_back(jv);
	}

	jc["operatorNodes"] = nlohmann::json::array();
	for (const auto& o : graph.operatorNodes)
	{
		nlohmann::ordered_json jo;
		jo["id"] = o.id;
		jo["posX"] = o.posX;
		jo["posY"] = o.posY;
		jo["op"] = ToString(o.op);
		jc["operatorNodes"].push_back(jo);
	}

	jc["logicNodes"] = nlohmann::json::array();
	for (const auto& l : graph.logicNodes)
	{
		nlohmann::ordered_json jl;
		jl["id"] = l.id;
		jl["posX"] = l.posX;
		jl["posY"] = l.posY;
		jl["op"] = ToString(l.op);
		jc["logicNodes"].push_back(jl);
	}

	jc["notNodes"] = nlohmann::json::array();
	for (const auto& n : graph.notNodes)
	{
		nlohmann::ordered_json jn;
		jn["id"] = n.id;
		jn["posX"] = n.posX;
		jn["posY"] = n.posY;
		jc["notNodes"].push_back(jn);
	}

	jc["getterNodes"] = nlohmann::json::array();
	for (const auto& g : graph.getterNodes)
	{
		nlohmann::ordered_json jg;
		jg["id"] = g.id;
		jg["posX"] = g.posX;
		jg["posY"] = g.posY;
		jg["componentName"] = g.componentName;
		jg["fieldName"] = g.fieldName;
		jg["selectedComponent"] = ToString(g.selectedComponent);
		jc["getterNodes"].push_back(jg);
	}

	jc["links"] = nlohmann::json::array();
	for (const auto& cl : graph.links)
	{
		nlohmann::ordered_json jcl;
		jcl["id"] = cl.id;
		jcl["fromId"] = cl.fromId;
		jcl["toId"] = cl.toId;
		jcl["toPinB"] = cl.toPinB;
		jc["links"].push_back(jcl);
	}

	return jc;
}

// JSONから復元
void ConditionGraphFromJson(ConditionGraph& graph, const nlohmann::json& jc)
{
	graph = ConditionGraph{};

	if (jc.contains("returnNode"))
	{
		graph.returnNode.id = jc["returnNode"].value("id", 1);
		graph.returnNode.posX = jc["returnNode"].value("posX", 500.0f);
		graph.returnNode.posY = jc["returnNode"].value("posY", 200.0f);
	}

	if (jc.contains("variableNodes"))
	{
		for (const auto& jv : jc["variableNodes"])
		{
			ConditionVariableNode v;
			v.id = jv.value("id", -1);
			v.posX = jv.value("posX", 0.0f);
			v.posY = jv.value("posY", 0.0f);
			v.name = jv.value("name", "");
			v.type = ToValueType(jv.value("type", "Float"));
			v.intValue = jv.value("intValue", 0);
			v.floatValue = jv.value("floatValue", 0.0f);
			v.boolValue = jv.value("boolValue", false);
			graph.variableNodes.push_back(v);
		}
	}

	if (jc.contains("operatorNodes"))
	{
		for (const auto& jo : jc["operatorNodes"])
		{
			ConditionOperatorNode o;
			o.id = jo.value("id", -1);
			o.posX = jo.value("posX", 0.0f);
			o.posY = jo.value("posY", 0.0f);
			o.op = ToCompareOp(jo.value("op", "Greater"));
			graph.operatorNodes.push_back(o);
		}
	}

	if (jc.contains("logicNodes"))
	{
		for (const auto& jl : jc["logicNodes"])
		{
			ConditionLogicNode l;
			l.id = jl.value("id", -1);
			l.posX = jl.value("posX", 0.0f);
			l.posY = jl.value("posY", 0.0f);
			l.op = ToLogicOp(jl.value("op", "And"));
			graph.logicNodes.push_back(l);
		}
	}

	if (jc.contains("notNodes"))
	{
		for (const auto& jn : jc["notNodes"])
		{
			ConditionNotNode n;
			n.id = jn.value("id", -1);
			n.posX = jn.value("posX", 0.0f);
			n.posY = jn.value("posY", 0.0f);
			graph.notNodes.push_back(n);
		}
	}

	if (jc.contains("getterNodes"))
	{
		for (const auto& jg : jc["getterNodes"])
		{
			ConditionGetterNode g;
			g.id = jg.value("id", -1);
			g.posX = jg.value("posX", 0.0f);
			g.posY = jg.value("posY", 0.0f);
			g.componentName = jg.value("componentName", "");
			g.fieldName = jg.value("fieldName", "");

			// 古い保存データは表示名で保存されているので、コード上の名前に置き換える
			if (!g.componentName.empty() && !g.fieldName.empty() &&
				!Reflection::FindField(g.componentName, g.fieldName))
			{
				if (const FieldEntry* f = Reflection::FindFieldByLabel(g.componentName, g.fieldName))
				{
					g.fieldName = f->fieldName;
				}
			}
			g.selectedComponent = ToFloatComponent(jg.value("selectedComponent", "None"));
			graph.getterNodes.push_back(g);
		}
	}

	if (jc.contains("links"))
	{
		for (const auto& jcl : jc["links"])
		{
			ConditionLink cl;
			cl.id = jcl.value("id", -1);
			cl.fromId = jcl.value("fromId", -1);
			cl.toId = jcl.value("toId", -1);
			cl.toPinB = jcl.value("toPinB", false);
			graph.links.push_back(cl);
		}
	}

	// 読み込んだIDの続きから採番する
	graph.RecalculateNextId();
}

// ゲッターが読むフィールドの型
ValueType GetGetterFieldType(const ConditionGetterNode& node)
{
	if (node.componentName.empty() || node.fieldName.empty()) return ValueType::Unsupported;

	const FieldEntry* field = Reflection::FindField(node.componentName, node.fieldName);
	return field ? field->type : ValueType::Unsupported;
}
// ゲッターで選んでいる成分が、その型で使えるか
bool IsGetterAxisValid(const ConditionGetterNode& node)
{
	int count = Reflection::GetComponentCount(GetGetterFieldType(node));
	int index = static_cast<int>(node.selectedComponent); // None=0, X=1, Y=2, Z=3, W=4
	return count > 0 && index >= 1 && index <= count;
}
