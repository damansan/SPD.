#include "StudioConditionCanvas.h"
#include <algorithm>
#include <string>
#include <utility>

#include "Engine/ECS/Reflection.h"
#include "StudioStateGraphDraw.h"

// このファイル内だけで使う処理
namespace
{
	// ピンの印(▶)
	const char* kPinIcon = "▶";

	// 各ノードの幅に合わせて、中央に配置する
	void CenterNextItem(float totalWidth, float itemWidth)
	{
		float pad = (totalWidth - itemWidth) * 0.5f;
		if (pad > 0.0f)
		{
			ImGui::Dummy(ImVec2(pad, 0.0f));
			ImGui::SameLine(0.0f, 0.0f);
		}
	}

	// 値の型ごとの枠の色
	ImColor GetValueTypeColor(ValueType type)
	{
		switch (type)
		{
		case ValueType::Int:    return ImColor(68, 201, 156, 255);  // 水色
		case ValueType::Float:
		case ValueType::Float2:
		case ValueType::Float3:
		case ValueType::Float4: return ImColor(147, 226, 74, 255);  // 緑
		case ValueType::Bool:   return ImColor(220, 48, 48, 255);   // 赤
		case ValueType::Unsupported: return ImColor(128, 128, 128, 255); // 灰色(条件で読めない)

		default:                return ImColor(255, 255, 255, 255);
		}
	}

	// 比較演算の記号
	const char* GetCompareOpSymbol(CompareOp op)
	{
		switch (op)
		{
		case CompareOp::Greater:      return ">";
		case CompareOp::Less:         return "<";
		case CompareOp::Equal:        return "==";
		case CompareOp::NotEqual:     return "!=";
		case CompareOp::GreaterEqual: return ">=";
		case CompareOp::LessEqual:    return "<=";
		default:                      return "?";
		}
	}

	// 論理演算の記号
	const char* GetLogicOpSymbol(LogicOp op)
	{
		return (op == LogicOp::Or) ? "OR" : "AND";
	}

	// 成分の名前
	const char* GetAxisName(FloatComponent c)
	{
		switch (c)
		{
		case FloatComponent::X: return "X";
		case FloatComponent::Y: return "Y";
		case FloatComponent::Z: return "Z";
		case FloatComponent::W: return "W";
		default:                return "?";
		}
	}

	// 値の型の名前
	const char* GetValueTypeName(ValueType type)
	{
		switch (type)
		{
		case ValueType::Int:         return "int";
		case ValueType::Float:       return "float";
		case ValueType::Bool:        return "bool";
		case ValueType::Float2:      return "float2";
		case ValueType::Float3:      return "float3";
		case ValueType::Float4:      return "float4";
		case ValueType::Unsupported: return "unsupported";
		default:                     return "?";
		}
	}

	// エラーの文字の色
	const ImVec4 kErrorTextColor(1.0f, 0.4f, 0.4f, 1.0f);

	// IDでノードを探す
	template<typename T>
	T* FindNode(std::vector<T>& list, int id)
	{
		for (auto& n : list)
		{
			if (n.id == id) return &n;
		}
		return nullptr;
	}

	// エラーの文字を、折り返して描く
	void ErrorTextWrapped(const char* text)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, kErrorTextColor);
		ImGui::TextWrapped("%s", text);
		ImGui::PopStyleColor();
	}

	// 記号を大きめにして中央に描く
	void DrawSymbol(const char* symbol, float nodeWidth, float scale)
	{
		ImGui::SetWindowFontScale(scale);
		CenterNextItem(nodeWidth, ImGui::CalcTextSize(symbol).x);
		ImGui::Text("%s", symbol);
		ImGui::SetWindowFontScale(1.0f);
	}

	// ピンを1つ描く
	void DrawPin(ed::PinId pinId, ed::PinKind kind)
	{
		ed::BeginPin(pinId, kind);
		ImGui::Text("%s", kPinIcon);
		ed::EndPin();
	}
}


void StudioConditionCanvas::Initialize()
{
	// ノードエディタのコンテキストを作る
	canvas.Initialize();
}

// 編集対象の条件グラフを設定する
void StudioConditionCanvas::SetTarget(ConditionGraph* target)
{
	graph = target;

	// 次に描くときに、データの位置をノードに反映し直す
	needsApplyPositions = true;

	// 前の条件の選択を消す
	clearSelectionRequested = true;
	selectedNodeIds.clear();
	selectedLinkId = -1;

	// ゲッターの一覧の状態も戻す
	getterInspectorNodeId = -1;
	browseComponent.clear();
}

// キャンバスを描く
void StudioConditionCanvas::Draw()
{
	// 編集する条件が無ければ、キャンバスを作らずに抜ける
	if (!graph)
	{
		ImGui::TextDisabled("No link selected.");
		return;
	}

	// キャンバス作成
	canvas.Begin("ConditionCanvas");

	// 削除や読み込みの後はノードエディタ側に残った選択を消す
	if (clearSelectionRequested)
	{
		ed::ClearSelection();
		clearSelectionRequested = false;
	}

	
	// ノード描画
	ed::PushStyleVar(ed::StyleVar_LinkStrength, 0.0f);	
	DrawReturnNode();
	DrawVariableNodes();
	DrawGetterNodes();
	DrawOperatorNodes();
	DrawLogicNodes();
	DrawNotNodes();
	ed::PopStyleVar(1);

	// リンク描画
	DrawLinks();

	// メニュー描画
	DrawCreateMenu();

	// 入力更新
	UpdateLinkCreation();
	UpdateSelection();

	// 位置の反映は、切り替えた直後の1フレームだけ
	needsApplyPositions = false;

	// キャンバス終了
	canvas.End();


	// 選択している物の削除
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
		!ImGui::GetIO().WantTextInput &&
		ImGui::IsKeyPressed(ImGuiKey_Delete))
	{
		DeleteSelection();
	}
}

// インスペクターを描く
void StudioConditionCanvas::DrawInspector()
{
	// 編集する条件が無ければ何も出さない
	if (!graph)
	{
		ImGui::TextDisabled("No link selected.");
		return;
	}

	// リンク描画
	if (selectedLinkId != -1)
	{
		for (const auto& l : graph->links)
		{
			if (l.id == selectedLinkId)
			{
				DrawLinkInspector(l);
				return;
			}
		}
		return;
	}

	// ノード複数選択
	if (selectedNodeIds.size() > 1)
	{
		ImGui::Text("%d nodes selected", static_cast<int>(selectedNodeIds.size()));
		ImGui::Separator();

		bool hasReturn = std::find(selectedNodeIds.begin(), selectedNodeIds.end(),
			graph->returnNode.id) != selectedNodeIds.end();
		if (hasReturn)
		{
			ImGui::TextDisabled("Return is not deleted.");
		}

		if (StateGraphDraw::DeleteButton("Delete Nodes"))
		{
			DeleteSelection();
		}
		return;
	}

	// ノード1つ選択
	if (selectedNodeIds.size() == 1)
	{
		int id = selectedNodeIds[0];
		switch (graph->GetNodeKind(id))
		{
		case ConditionNodeKind::Return:
			// Returnは条件に必ず1つあるので、削除ボタンは出さない
			DrawReturnInspector();
			return;

		case ConditionNodeKind::Variable: DrawVariableInspector(*FindNode(graph->variableNodes, id)); break;
		case ConditionNodeKind::Getter:   DrawGetterInspector  (*FindNode(graph->getterNodes,   id)); break;
		case ConditionNodeKind::Operator: DrawOperatorInspector(*FindNode(graph->operatorNodes, id)); break;
		case ConditionNodeKind::Logic:    DrawLogicInspector   (*FindNode(graph->logicNodes,    id)); break;
		case ConditionNodeKind::Not:      DrawNotInspector();                                         break;
		default: return;
		}

		// Return以外は削除できる
		if (StateGraphDraw::DeleteButton("Delete Node"))
		{
			DeleteSelection();
		}
		return;
	}

	// 未選択
	ImGui::TextDisabled("No node selected.");
}

// リターンノード描画
void StudioConditionCanvas::DrawReturnNode()
{
	ConditionReturnNode& node = graph->returnNode;

	// 読み込み直後はデータの位置にノードを反映
	ApplyPosition(node.id, node.posX, node.posY);

	// Returnは紫で描く
	ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(197, 134, 192, 255));
	ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(140, 90, 137, 255));

	// 左に入力ピン、右に名前
	ed::BeginNode(ed::NodeId(static_cast<uint64_t>(node.id)));
	DrawPin(ToPinId(node.id, kPinInA), ed::PinKind::Input);
	ImGui::SameLine();
	CenterNextItem(100.0f, ImGui::CalcTextSize("Return").x);
	ImGui::Text("Return");
	ed::EndNode();

	ed::PopStyleColor(2);

	// ノードを動かしたら、データの位置に書き戻す
	ReadPosition(node.id, node.posX, node.posY);
}

// 変数ノード描画
void StudioConditionCanvas::DrawVariableNodes()
{
	const float nodeWidth = 140.0f;
	const float valueWidth = 80.0f;

	for (auto& v : graph->variableNodes)
	{
		// 読み込み直後はデータの位置にノードを反映
		ApplyPosition(v.id, v.posX, v.posY);

		// 型ごとに枠の色を変える
		ed::PushStyleColor(ed::StyleColor_NodeBorder, GetValueTypeColor(v.type));
		ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, 2.0f);

		ed::BeginNode(ed::NodeId(static_cast<uint64_t>(v.id)));
		ImGui::PushID(v.id);

		// 1行目：名前と出力ピン
		std::string name = v.name.empty() ? "Variable" : v.name;
		CenterNextItem(nodeWidth, ImGui::CalcTextSize(name.c_str()).x);
		ImGui::Text("%s", name.c_str());
		ImGui::SameLine(nodeWidth);
		DrawPin(ToPinId(v.id, kPinOut), ed::PinKind::Output);

		// 2行目：値
		if (v.type == ValueType::Int)
		{
			CenterNextItem(nodeWidth, valueWidth);
			ImGui::SetNextItemWidth(valueWidth);
			ImGui::DragInt("##int", &v.intValue, 1);
		}
		else if (v.type == ValueType::Float)
		{
			CenterNextItem(nodeWidth, valueWidth);
			ImGui::SetNextItemWidth(valueWidth);
			ImGui::DragFloat("##float", &v.floatValue, 0.1f);
		}
		else
		{
			CenterNextItem(nodeWidth, ImGui::GetFrameHeight());
			ImGui::Checkbox("##bool", &v.boolValue);
		}

		ImGui::PopID();
		ed::EndNode();

		ed::PopStyleColor(1);
		ed::PopStyleVar(1);

		// ノードを動かしたら、データの位置に書き戻す
		ReadPosition(v.id, v.posX, v.posY);
	}
}

// ゲッターノード描画
void StudioConditionCanvas::DrawGetterNodes()
{
	const float spacing = 8.0f;

	for (auto& g : graph->getterNodes)
	{
		ApplyPosition(g.id, g.posX, g.posY);

		bool assigned = !g.componentName.empty() && !g.fieldName.empty();
		const FieldEntry* field = assigned ? Reflection::FindField(g.componentName, g.fieldName) : nullptr;

		// 1行目：コンポーネント名
		std::string line1;
		// 2行目：フィールドの表示名
		std::string line2;

		ImColor border;
		if (!assigned)
		{
			// フィールド未設定
			line1 = "Empty";
			border = ImColor(255, 255, 255, 255);
		}
		else if (!field)
		{
			// 名前を変えた・消した等で、今のコードに無いフィールド
			line1 = g.componentName;
			line2 = g.fieldName + " (missing)";
			border = ImColor(230, 90, 90, 255);
		}
		else
		{
			// 設定済み
			line1 = g.componentName;
			line2 = field->fieldLabel;
			if (Reflection::GetComponentCount(field->type) > 0)
			{
				line2 += std::string(".") + GetAxisName(g.selectedComponent);
			}
			border = GetValueTypeColor(field->type);
		}

		ed::PushStyleColor(ed::StyleColor_NodeBorder, border);
		ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, 2.0f);

		ed::BeginNode(ed::NodeId(static_cast<uint64_t>(g.id)));
		ImGui::PushID(g.id);

		// 長いほうの行に合わせてノードの幅を決める
		float width1 = ImGui::CalcTextSize(line1.c_str()).x;
		float width2 = ImGui::CalcTextSize(line2.c_str()).x;
		float nodeWidth = (std::max)(width1, width2) + spacing;

		// 文字を中央に描き、右端に出力ピンを置く
		CenterNextItem(nodeWidth, width1);
		ImGui::Text("%s", line1.c_str());
		if (!line2.empty())
		{
			CenterNextItem(nodeWidth, width2);
			ImGui::Text("%s", line2.c_str());
		}
		ImGui::SameLine(nodeWidth);
		DrawPin(ToPinId(g.id, kPinOut), ed::PinKind::Output);

		ImGui::PopID();
		ed::EndNode();

		ed::PopStyleColor(1);
		ed::PopStyleVar(1);

		// ノードを動かしたら、データの位置に書き戻す
		ReadPosition(g.id, g.posX, g.posY);
	}
}

// 比較演算ノード描画
void StudioConditionCanvas::DrawOperatorNodes()
{
	const float nodeWidth = 100.0f;

	for (auto& o : graph->operatorNodes)
	{
		// 切り替えた直後だけ、データの位置をノードに反映
		ApplyPosition(o.id, o.posX, o.posY);

		// 比較は青で描く
		ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(90, 140, 220, 255));
		ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, 2.0f);

		ed::BeginNode(ed::NodeId(static_cast<uint64_t>(o.id)));
		ImGui::PushID(o.id);

		// 1行目：入力A
		DrawPin(ToPinId(o.id, kPinInA), ed::PinKind::Input);
		// 2行目：記号
		DrawSymbol(GetCompareOpSymbol(o.op), nodeWidth, 2.0f);
		// 3行目：入力Bと出力
		DrawPin(ToPinId(o.id, kPinInB), ed::PinKind::Input);
		ImGui::SameLine(nodeWidth);
		DrawPin(ToPinId(o.id, kPinOut), ed::PinKind::Output);

		ImGui::PopID();
		ed::EndNode();

		ed::PopStyleColor(1);
		ed::PopStyleVar(1);

		// ノードを動かしたら、データの位置に書き戻す
		ReadPosition(o.id, o.posX, o.posY);
	}
}

// 論理演算ノード描画
void StudioConditionCanvas::DrawLogicNodes()
{
	const float nodeWidth = 100.0f;

	for (auto& l : graph->logicNodes)
	{
		// 切り替えた直後だけ、データの位置をノードに反映
		ApplyPosition(l.id, l.posX, l.posY);

		// 論理はオレンジで描く
		ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(200, 130, 60, 255));
		ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, 2.0f);

		ed::BeginNode(ed::NodeId(static_cast<uint64_t>(l.id)));
		ImGui::PushID(l.id);

		// 1行目：入力A
		DrawPin(ToPinId(l.id, kPinInA), ed::PinKind::Input);
		// 2行目：記号
		DrawSymbol(GetLogicOpSymbol(l.op), nodeWidth, 1.5f);
		// 3行目：入力Bと出力
		DrawPin(ToPinId(l.id, kPinInB), ed::PinKind::Input);
		ImGui::SameLine(nodeWidth);
		DrawPin(ToPinId(l.id, kPinOut), ed::PinKind::Output);

		ImGui::PopID();
		ed::EndNode();

		ed::PopStyleColor(1);
		ed::PopStyleVar(1);

		// ノードを動かしたら、データの位置に書き戻す
		ReadPosition(l.id, l.posX, l.posY);
	}
}

// 否定ノード描画
void StudioConditionCanvas::DrawNotNodes()
{
	const float nodeWidth = 80.0f;

	for (auto& n : graph->notNodes)
	{
		// 切り替えた直後だけ、データの位置をノードに反映
		ApplyPosition(n.id, n.posX, n.posY);

		// 否定は赤で描く
		ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(200, 80, 80, 255));
		ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, 2.0f);

		ed::BeginNode(ed::NodeId(static_cast<uint64_t>(n.id)));
		ImGui::PushID(n.id);

		// 1行目：記号
		DrawSymbol("NOT", nodeWidth, 1.5f);
		// 2行目：入力と出力
		DrawPin(ToPinId(n.id, kPinInA), ed::PinKind::Input);
		ImGui::SameLine(nodeWidth);
		DrawPin(ToPinId(n.id, kPinOut), ed::PinKind::Output);

		ImGui::PopID();
		ed::EndNode();

		ed::PopStyleColor(1);
		ed::PopStyleVar(1);

		// ノードを動かしたら、データの位置に書き戻す
		ReadPosition(n.id, n.posX, n.posY);
	}
}

// リンク描画
void StudioConditionCanvas::DrawLinks()
{
	const ImColor validColor(210, 210, 210, 255);
	const ImColor invalidColor(230, 90, 90, 255);

	for (const auto& l : graph->links)
	{
		// 両端のノードが無いリンクは描かない
		if (graph->GetNodeKind(l.fromId) == ConditionNodeKind::None) continue;
		if (graph->GetNodeKind(l.toId) == ConditionNodeKind::None) continue;

		// 型を変えた等で、決まりに合わなくなったリンクは赤
		ImColor color = graph->IsLinkValid(l) ? validColor : invalidColor;

		// 出す側は必ず出力ピン、受ける側はtoPinBでAかBを選ぶ
		ed::Link(
			ed::LinkId(static_cast<uint64_t>(l.id)),
			ToPinId(l.fromId, kPinOut),
			ToPinId(l.toId, l.toPinB ? kPinInB : kPinInA),
			color, 2.0f);
	}
}

// 背景右クリックのノード作成メニュー
void StudioConditionCanvas::DrawCreateMenu()
{
	// メニューは画面の座標で描く
	ed::Suspend();

	// 背景を右クリックしたら、位置を覚えてメニューを開く
	if (ed::ShowBackgroundContextMenu())
	{
		pendingCreatePos = ImGui::GetMousePos();
		ImGui::OpenPopup("CreateConditionNode");
	}

	// ポップ内容
	if (ImGui::BeginPopup("CreateConditionNode"))
	{
		// 右クリックした位置(キャンバスの座標)
		ImVec2 pos = ed::ScreenToCanvas(pendingCreatePos);
		int newId = -1;

		// 変数
		if (ImGui::BeginMenu("Add Variable"))
		{
			if (ImGui::MenuItem("int"))   newId = graph->AddVariableNode(ValueType::Int, pos.x, pos.y);
			if (ImGui::MenuItem("float")) newId = graph->AddVariableNode(ValueType::Float, pos.x, pos.y);
			if (ImGui::MenuItem("bool"))  newId = graph->AddVariableNode(ValueType::Bool, pos.x, pos.y);
			ImGui::EndMenu();
		}

		// 比較
		if (ImGui::BeginMenu("Add Operator"))
		{
			if (ImGui::MenuItem(">"))  newId = graph->AddOperatorNode(CompareOp::Greater, pos.x, pos.y);
			if (ImGui::MenuItem("<"))  newId = graph->AddOperatorNode(CompareOp::Less, pos.x, pos.y);
			if (ImGui::MenuItem("==")) newId = graph->AddOperatorNode(CompareOp::Equal, pos.x, pos.y);
			if (ImGui::MenuItem("!=")) newId = graph->AddOperatorNode(CompareOp::NotEqual, pos.x, pos.y);
			if (ImGui::MenuItem(">=")) newId = graph->AddOperatorNode(CompareOp::GreaterEqual, pos.x, pos.y);
			if (ImGui::MenuItem("<=")) newId = graph->AddOperatorNode(CompareOp::LessEqual, pos.x, pos.y);
			ImGui::EndMenu();
		}

		// 論理
		if (ImGui::BeginMenu("Add Logic"))
		{
			if (ImGui::MenuItem("AND")) newId = graph->AddLogicNode(LogicOp::And, pos.x, pos.y);
			if (ImGui::MenuItem("OR"))  newId = graph->AddLogicNode(LogicOp::Or, pos.x, pos.y);
			if (ImGui::MenuItem("NOT")) newId = graph->AddNotNode(pos.x, pos.y);
			ImGui::EndMenu();
		}

		// コンポーネントの値
		if (ImGui::MenuItem("Add Getter"))
		{
			newId = graph->AddGetterNode(pos.x, pos.y);
		}

		// 作ったノードを、右クリックした位置に置く
		if (newId != -1)
		{
			ed::SetNodePosition(ed::NodeId(static_cast<uint64_t>(newId)), pos);
		}

		ImGui::EndPopup();
	}

	ed::Resume();
}

// ピンからドラッグしてリンクを作る
void StudioConditionCanvas::UpdateLinkCreation()
{
	// ドラッグ中の処理
	if (ed::BeginCreate(ImColor(255, 255, 255, 255), 2.0f))
	{
		// ドラッグ中の2つのピン
		ed::PinId pinA, pinB;
		if (ed::QueryNewLink(&pinA, &pinB))
		{
			// ピンのIDを、ノードのIDとピンの位置に戻す
			int nodeA = -1, nodeB = -1;
			PinSlot slotA = kPinOut, slotB = kPinOut;
			bool decoded = FromPinId(pinA, nodeA, slotA) && FromPinId(pinB, nodeB, slotB);

			// ドラッグの向きに関わらず、A＝出力、B＝入力にそろえる
			if (decoded && slotA != kPinOut)
			{
				std::swap(nodeA, nodeB);
				std::swap(slotA, slotB);
			}

			// つないでよいかを判断する
			std::string reason;
			bool ok = false;
			if (!decoded || slotA != kPinOut || slotB == kPinOut)
			{
				// 出力同士・入力同士
				reason = "Connect an output to an input.";
			}
			else
			{
				ok = graph->CanLink(nodeA, nodeB, slotB == kPinInB, &reason);
			}

			if (ok)
			{
				// マウスを離した瞬間にtrueになる
				if (ed::AcceptNewItem(ImColor(255, 255, 255, 255), 2.0f))
				{
					graph->AddLink(nodeA, nodeB, slotB == kPinInB);
				}
			}
			else
			{
				// つなげられない理由を、マウスの近くに出す
				ed::Suspend();
				ImGui::SetTooltip("%s", reason.c_str());
				ed::Resume();

				// 線を赤くして、離してもつながらないようにする
				ed::RejectNewItem(ImColor(255, 0, 0, 255), 2.0f);
			}
		}
	}
	ed::EndCreate();
}

// 選択の更新
void StudioConditionCanvas::UpdateSelection()
{
	// 選ばれているノードとリンクを、ノードエディタから受け取る
	int count = ed::GetSelectedObjectCount();
	std::vector<ed::NodeId> nodeIds(count > 0 ? count : 1);
	std::vector<ed::LinkId> linkIds(count > 0 ? count : 1);
	int nodeCount = (count > 0) ? ed::GetSelectedNodes(nodeIds.data(), count) : 0;
	int linkCount = (count > 0) ? ed::GetSelectedLinks(linkIds.data(), count) : 0;

	// ノードの選択(削除した直後などで、データに無いものは除く)
	selectedNodeIds.clear();
	for (int i = 0; i < nodeCount; ++i)
	{
		int id = static_cast<int>(nodeIds[i].Get());
		if (graph->GetNodeKind(id) != ConditionNodeKind::None)
		{
			selectedNodeIds.push_back(id);
		}
	}

	// リンクの選択(ノードが選ばれていないときだけ、最初の1本)
	selectedLinkId = -1;
	if (selectedNodeIds.empty() && linkCount > 0)
	{
		int id = static_cast<int>(linkIds[0].Get());
		bool exists = std::any_of(graph->links.begin(), graph->links.end(),
			[id](const ConditionLink& l) { return l.id == id; });
		if (exists) selectedLinkId = id;
	}
}

// 選択している物を削除
void StudioConditionCanvas::DeleteSelection()
{
	// リンクを選んでいれば、そのリンクを消す
	if (selectedLinkId != -1)
	{
		graph->RemoveLink(selectedLinkId);
	}

	// 選んでいるノードを消す
	for (int id : selectedNodeIds)
	{
		graph->RemoveNode(id);
	}

	selectedNodeIds.clear();
	selectedLinkId = -1;

	// ノードエディタ側に残った、消えたノードの選択も消す
	clearSelectionRequested = true;
}

// リターンノードの設定
void StudioConditionCanvas::DrawReturnInspector()
{
	ImGui::Text("Return");
	ImGui::Separator();

	// 何もつながっていなければ条件が空なことを出す
	if (graph->IsEmpty())
	{
		ImGui::Spacing();
		ErrorTextWrapped("Not connected. This condition is empty.");
	}

	// Returnは条件に必ず1つあるので、削除できないことを知らせる
	ImGui::Spacing();
	ImGui::TextDisabled("Return cannot be deleted.");
}

// 変数ノードの設定
void StudioConditionCanvas::DrawVariableInspector(ConditionVariableNode& node)
{
	ImGui::Text("Variable");
	ImGui::Separator();

	// 名前(空ならノードに"Variable"と表示)
	char nameBuffer[128] = {};
	strncpy_s(nameBuffer, node.name.c_str(), sizeof(nameBuffer) - 1);
	if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
	{
		node.name = nameBuffer;
	}

	// 型は作るときに決まる(変えるとつながっているリンクの意味が変わるので、表示だけ)
	ImGui::Text("Type: %s", GetValueTypeName(node.type));

	// 値
	switch (node.type)
	{
	case ValueType::Int:   ImGui::DragInt("Value", &node.intValue, 1);       break;
	case ValueType::Float: ImGui::DragFloat("Value", &node.floatValue, 0.1f); break;
	case ValueType::Bool:  ImGui::Checkbox("Value", &node.boolValue);        break;
	default: break;
	}
}

// ゲッターノードの設定
void StudioConditionCanvas::DrawGetterInspector(ConditionGetterNode& node)
{
	const auto& fields = Reflection::GetAllFields();

	// 表示するゲッターが変わったら、一覧を今の設定に合わせる
	if (getterInspectorNodeId != node.id)
	{
		getterInspectorNodeId = node.id;
		browseComponent = node.componentName;
		scrollToCurrent = true;
	}

	ImGui::Text("Getter");
	ImGui::Separator();

	// 今の設定
	bool assigned = !node.componentName.empty() && !node.fieldName.empty();
	const FieldEntry* currentField = assigned ? Reflection::FindField(node.componentName, node.fieldName) : nullptr;
	if (!assigned)
	{
		ImGui::TextDisabled("No field selected.");
	}
	else if (!currentField)
	{
		ImGui::TextColored(kErrorTextColor, "%s.%s (missing)", node.componentName.c_str(), node.fieldName.c_str());
	}
	else
	{
		ImGui::Text("%s.%s (%s)", node.componentName.c_str(), currentField->fieldLabel.c_str(), GetValueTypeName(currentField->type));
	}
	ImGui::Spacing();

	// 1段目：コンポーネント(選んでも一覧が切り替わるだけで、データは変えない)
	ImGui::Text("Component");
	ImGui::BeginChild("##GetterComponents", ImVec2(0.0f, 120.0f), true);
	std::string previous;
	for (const auto& f : fields)
	{
		// 同じコンポーネントのフィールドは続けて並んでいるので、名前が変わったところで1行出す
		if (f.componentName == previous) continue;
		previous = f.componentName;

		bool open = (f.componentName == browseComponent);
		if (ImGui::Selectable(f.componentName.c_str(), open))
		{
			browseComponent = f.componentName;
		}
		if (open && scrollToCurrent)
		{
			ImGui::SetScrollHereY(0.5f);
		}
	}
	ImGui::EndChild();

	// 2段目：フィールド(選んだときにデータを変える)
	ImGui::Text("Field");
	ImGui::BeginChild("##GetterFields", ImVec2(0.0f, 200.0f), true);
	if (browseComponent.empty())
	{
		ImGui::TextDisabled("Select a component.");
	}
	for (const auto& f : fields)
	{
		if (f.componentName != browseComponent) continue;

		// 条件で読めない型は、灰色で選べないようにする
		bool readable = (f.type != ValueType::Unsupported);
		bool current = (f.componentName == node.componentName && f.fieldName == node.fieldName);
		std::string label = f.fieldLabel + "  (" + GetValueTypeName(f.type) + ")";

		ImGui::PushID(f.fieldName.c_str());
		ImGuiSelectableFlags flags = readable ? ImGuiSelectableFlags_None : ImGuiSelectableFlags_Disabled;
		if (ImGui::Selectable(label.c_str(), current, flags) && !current)
		{
			node.componentName = f.componentName;
			node.fieldName = f.fieldName;
			node.selectedComponent = FloatComponent::None; // 成分は選び直す
		}
		if (current && scrollToCurrent)
		{
			ImGui::SetScrollHereY(0.5f);
		}
		ImGui::PopID();
	}
	ImGui::EndChild();
	scrollToCurrent = false;

	// 3段目：成分(Float2～4のときだけ)
	// フィールドをこのフレームで選び直した場合に備えて、型は選んだ後に求め直す
	ValueType fieldType = GetGetterFieldType(node);
	int count = Reflection::GetComponentCount(fieldType);
	if (count > 0)
	{
		static const char* names[] = { "X", "Y", "Z", "W" };
		static const FloatComponent values[] = { FloatComponent::X, FloatComponent::Y, FloatComponent::Z, FloatComponent::W };

		ImGui::Spacing();
		ImGui::Text("Axis");
		for (int i = 0; i < count; ++i)
		{
			if (i > 0) ImGui::SameLine();
			if (ImGui::RadioButton(names[i], node.selectedComponent == values[i]))
			{
				node.selectedComponent = values[i];
			}
		}

		// 成分を選ぶまでつなげない
		if (!IsGetterAxisValid(node))
		{
			ImGui::TextColored(kErrorTextColor, "Select an axis to connect.");
		}
	}
}

// 比較演算ノードの設定
void StudioConditionCanvas::DrawOperatorInspector(ConditionOperatorNode& node)
{
	ImGui::Text("Operator");
	ImGui::Separator();

	static const CompareOp ops[] =
	{
		CompareOp::Greater, CompareOp::Less, CompareOp::Equal,
		CompareOp::NotEqual, CompareOp::GreaterEqual, CompareOp::LessEqual,
	};

	// 6種類のどれにでも変えられる(合わなくなったリンクは赤になる)
	for (int i = 0; i < 6; ++i)
	{
		if (i % 3 != 0) ImGui::SameLine();
		if (ImGui::RadioButton(GetCompareOpSymbol(ops[i]), node.op == ops[i]))
		{
			node.op = ops[i];
		}
	}

	ImGui::Spacing();
	ImGui::TextDisabled("A and B must be the same type.");
	ImGui::TextDisabled("Bool: == or != only.");
}

// 論理演算ノードの設定
void StudioConditionCanvas::DrawLogicInspector(ConditionLogicNode& node)
{
	ImGui::Text("Logic");
	ImGui::Separator();

	if (ImGui::RadioButton("AND", node.op == LogicOp::And)) node.op = LogicOp::And;
	ImGui::SameLine();
	if (ImGui::RadioButton("OR", node.op == LogicOp::Or))   node.op = LogicOp::Or;

	ImGui::Spacing();
	ImGui::TextDisabled("A and B must be bool.");
}

// 否定ノードの設定
void StudioConditionCanvas::DrawNotInspector()
{
	ImGui::Text("Not");
	ImGui::Separator();

	ImGui::TextWrapped("Inverts the input bool.");
}

// リンクの設定
void StudioConditionCanvas::DrawLinkInspector(const ConditionLink& link)
{
	ImGui::Text("Link");
	ImGui::Separator();

	// 入力が2つあるノードは、AとBのどちらにつながっているかも出す
	ConditionNodeKind toKind = graph->GetNodeKind(link.toId);
	bool twoInputs = (toKind == ConditionNodeKind::Operator || toKind == ConditionNodeKind::Logic);
	const char* pin = twoInputs ? (link.toPinB ? " (B)" : " (A)") : "";

	ImGui::Text("%s", GetNodeName(link.fromId).c_str());
	ImGui::Text("-> %s%s", GetNodeName(link.toId).c_str(), pin);

	// 決まりに合わないリンクは、赤い理由を出す(Validateでもエラーになる)
	std::string reason;
	if (!graph->IsLinkValid(link, &reason))
	{
		ImGui::Spacing();
		ErrorTextWrapped(("Invalid: " + reason).c_str());
	}

	if (StateGraphDraw::DeleteButton("Delete Link"))
	{
		DeleteSelection();
	}
}

// 表示用ノードの名前
std::string StudioConditionCanvas::GetNodeName(int nodeId) const
{
	switch (graph->GetNodeKind(nodeId))
	{
	case ConditionNodeKind::Return:
		return "Return";

	case ConditionNodeKind::Variable:
	{
		const ConditionVariableNode* v = FindNode(graph->variableNodes, nodeId);
		return v->name.empty() ? "Variable" : v->name;
	}

	case ConditionNodeKind::Getter:
	{
		const ConditionGetterNode* g = FindNode(graph->getterNodes, nodeId);
		if (g->componentName.empty() || g->fieldName.empty()) return "Getter (Empty)";

		const FieldEntry* field = Reflection::FindField(g->componentName, g->fieldName);
		if (!field) return g->componentName + "." + g->fieldName + " (missing)";

		std::string name = g->componentName + "." + field->fieldLabel;
		if (Reflection::GetComponentCount(field->type) > 0)
		{
			name += std::string(".") + GetAxisName(g->selectedComponent);
		}
		return name;
	}

	case ConditionNodeKind::Operator:
		return std::string("Compare ") + GetCompareOpSymbol(FindNode(graph->operatorNodes, nodeId)->op);

	case ConditionNodeKind::Logic:
		return GetLogicOpSymbol(FindNode(graph->logicNodes, nodeId)->op);

	case ConditionNodeKind::Not:
		return "NOT";

	default:
		return "(missing)";
	}
}

// 対象を切り替えた直後だけ、データの位置をノードに反映する
void StudioConditionCanvas::ApplyPosition(int nodeId, float x, float y) const
{
	if (!needsApplyPositions) return;
	ed::SetNodePosition(ed::NodeId(static_cast<uint64_t>(nodeId)), ImVec2(x, y));
}

// ノードを動かしたら、データの位置に書き戻す
void StudioConditionCanvas::ReadPosition(int nodeId, float& x, float& y)
{
	ImVec2 pos = ed::GetNodePosition(ed::NodeId(static_cast<uint64_t>(nodeId)));
	x = pos.x;
	y = pos.y;
}

// ノードのIDと位置 <-> ピンのID
ed::PinId StudioConditionCanvas::ToPinId(int nodeId, PinSlot slot)
{
	// 一度uint32_tにしてから広げる(負の値が入っても上位ビットが埋まらないように)
	uint64_t id = static_cast<uint64_t>(static_cast<uint32_t>(nodeId));
	return ed::PinId(kPinIdTag | (id << 2) | static_cast<uint64_t>(slot));
}
bool StudioConditionCanvas::FromPinId(ed::PinId pinId, int& outNodeId, PinSlot& outSlot)
{
	uint64_t value = pinId.Get();

	// 印が無ければ、このキャンバスのピンではない
	if ((value & kPinIdTag) == 0) return false;
	value &= ~kPinIdTag;

	// 位置0は使っていない
	uint64_t slot = value & 0x3;
	if (slot == 0) return false;

	outNodeId = static_cast<int>(value >> 2);
	outSlot = static_cast<PinSlot>(slot);
	return true;
}
