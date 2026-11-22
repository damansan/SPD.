#pragma once
#include <string>
#include <vector>
#include <json.hpp>
#include "Engine/ECS/Reflection.h"

// 構造体のどの成分を取り出すか
enum class FloatComponent
{
	None,
	X, Y, Z, W,
};

// 比較演算子の種類
enum class CompareOp
{
	Greater,      // >
	Less,         // <
	Equal,        // ==
	NotEqual,     // !=
	GreaterEqual, // >=
	LessEqual,    // <=
};

// 論理演算子の種類
enum class LogicOp
{
	And, // かつ
	Or,  // または
};

// コンポーネントの値を取得するノード
struct ConditionGetterNode
{
	int id = -1;
	float posX = 0.0f;
	float posY = 0.0f;
	std::string componentName; // 型名
	std::string fieldName;     // フィールドのコード上の名前
	FloatComponent selectedComponent = FloatComponent::None;
};

// 変数ノード
struct ConditionVariableNode
{
	int id = -1;
	float posX = 0.0f;
	float posY = 0.0f;
	std::string name; // 空の間は"Variable"と表示
	ValueType type = ValueType::Float;
	int intValue = 0;
	float floatValue = 0.0f;
	bool boolValue = false;
};

// 比較ノード
struct ConditionOperatorNode
{
	int id = -1;
	float posX = 0.0f;
	float posY = 0.0f;
	CompareOp op = CompareOp::Greater;
};

// 論理演算ノード
struct ConditionLogicNode
{
	int id = -1;
	float posX = 0.0f;
	float posY = 0.0f;
	LogicOp op = LogicOp::And;
};

// 否定ノード
struct ConditionNotNode
{
	int id = -1;
	float posX = 0.0f;
	float posY = 0.0f;
};

// 条件グラフの終着点
struct ConditionReturnNode
{
	int id = -1;
	float posX = 500.0f;
	float posY = 200.0f;
};

// 条件グラフのノードの種類
enum class ConditionNodeKind
{
	None,     // 見つからない
	Return,
	Variable,
	Getter,
	Operator,
	Logic,
	Not,
};

// 条件グラフ内のリンク
struct ConditionLink
{
	int id = -1;
	int fromId = -1;
	int toId = -1;
	bool toPinB = false;
};


// 遷移1本分の条件グラフ
struct ConditionGraph
{
	ConditionReturnNode returnNode;
	std::vector<ConditionVariableNode> variableNodes;
	std::vector<ConditionGetterNode>   getterNodes;
	std::vector<ConditionOperatorNode> operatorNodes;
	std::vector<ConditionLogicNode>    logicNodes;
	std::vector<ConditionNotNode>      notNodes;
	std::vector<ConditionLink>         links;

	// 次に割り当てるID(この条件グラフの中での採番)
	int nextId = 2;

	ConditionGraph() { returnNode.id = 1; }

	int GenerateId() { return nextId++; }

	// 使用中のIDの最大値+1に、採番をそろえる(読み込み後などに呼ぶ)
	void RecalculateNextId();

	// Returnに何もつながっていないか(条件が空か)
	bool IsEmpty() const;

	// ノードの種類
	ConditionNodeKind GetNodeKind(int nodeId) const;
	// 出力ピンが出す値の型
	bool GetOutputType(int nodeId, ValueType& outType) const;

	// 変数ノードの追加
	int AddVariableNode(ValueType type, float x, float y);
	// ゲッターノードの追加
	int AddGetterNode(float x, float y);
	// 比較ノード追加
	int AddOperatorNode(CompareOp op, float x, float y);
	// 論理演算ノード追加
	int AddLogicNode(LogicOp op, float x, float y);
	// 否定ノード追加
	int AddNotNode(float x, float y);

	// ノードの削除
	void RemoveNode(int nodeId);

	// リンクを作れるか
	bool CanLink(int fromNodeId, int toNodeId, bool toPinB, std::string* reason = nullptr) const;
	// リンクを作る
	int AddLink(int fromNodeId, int toNodeId, bool toPinB);
	
	// リンクの削除
	void RemoveLink(int linkId);

	// 今の設定で、リンクが決まりに合っているか
	bool IsLinkValid(const ConditionLink& link, std::string* reason = nullptr) const;

private:
	// リンクの決まりの確認
	bool CheckLink(int fromNodeId, int toNodeId, bool toPinB, int ignoreLinkId, std::string* reason) const;
	// nodeIdの入力を後ろへたどって、targetIdにたどり着くか
	bool DependsOn(int nodeId, int targetId, int ignoreLinkId) const;
};

// JSONとの変換
nlohmann::ordered_json ConditionGraphToJson(const ConditionGraph& graph);
void ConditionGraphFromJson(ConditionGraph& graph, const nlohmann::json& j);

// ゲッターが読むフィールドの型
ValueType GetGetterFieldType(const ConditionGetterNode& node);
// ゲッターで選んでいる成分が、その型で使えるか
bool IsGetterAxisValid(const ConditionGetterNode& node);