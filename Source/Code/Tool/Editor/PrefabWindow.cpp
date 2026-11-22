#include "PrefabWindow.h"
#include <imgui.h>
#include <algorithm>
#include "entt.hpp"
#include "Engine/Resource/EntitySerializer.h"

// 初期化
void PrefabWindow::Initialize(const std::string& root)
{
	rootPath = root;
	itemState.selectedPath = rootPath;

	if (!std::filesystem::exists(rootPath))
	{
		std::filesystem::create_directories(rootPath);
	}
}

// 描画
void PrefabWindow::Draw(entt::registry& registry, EditorContext& context)
{
	RefreshFolderTreeIfNeeded();
	RefreshFileGridIfNeeded();

	ImGui::Begin("Prefab");

	if (ImGui::BeginTable("PrefabLayout", 2,
		ImGuiTableFlags_Resizable |
		ImGuiTableFlags_BordersInnerV))
	{
		ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, 200.0f);
		ImGui::TableSetupColumn("Files", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();

		ImGui::TableSetColumnIndex(0);
		ImGui::BeginChild("PrefabFolderTree", ImVec2(0, 0), false);
		DrawFolderTree(folderTreeCache, registry, context);
		ImGui::EndChild();

		ImGui::TableSetColumnIndex(1);
		ImGui::BeginChild("PrefabFileGrid", ImVec2(0, 0), false);
		DrawFileGrid(registry, context);
		ImGui::EndChild();

		ImGui::EndTable();
	}

	DrawDeleteConfirmPopup();

	ImGui::End();
}

// 重複しない名前で作成
std::string PrefabWindow::GenerateUniqueName(const std::filesystem::path& parentDir, const std::string& baseName)
{
	std::filesystem::path candidate = parentDir / baseName;
	if (!std::filesystem::exists(candidate))
	{
		return baseName;
	}

	std::filesystem::path basePath(baseName);
	std::string stem = basePath.stem().string();
	std::string ext = basePath.extension().string();

	int suffix = 2;
	while (true)
	{
		std::string name = stem + " (" + std::to_string(suffix) + ")" + ext;
		if (!std::filesystem::exists(parentDir / name))
		{
			return name;
		}
		++suffix;
	}
}


// 左：フォルダツリー
void PrefabWindow::DrawFolderTree(FolderNode& node, entt::registry& registry, EditorContext& context)
{
	const std::filesystem::path& path = node.path;
	bool isRenamingThis = (itemState.mode == PrefabItemMode::Renaming && itemState.targetPath == path);

	bool hasSubFolder = !node.children.empty();

	// リネーム中はツリー行の代わりに入力欄を表示する
	if (isRenamingThis)
	{
		ImGui::SetNextItemWidth(160.0f);
		ImGui::SetKeyboardFocusHere();

		bool confirmedByEnter = ImGui::InputText("##rename", itemState.editBuffer, sizeof(itemState.editBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
		bool confirmedByBlur = ImGui::IsItemDeactivated() && !confirmedByEnter;

		if (confirmedByEnter || confirmedByBlur)
		{
			CommitRename();
		}
		// リネーム中はサブフォルダの再帰描画を行わない
		return;
	}

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;

	if (itemState.selectedPath == path)
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	if (!hasSubFolder)
	{
		flags |= ImGuiTreeNodeFlags_Leaf;
		flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	bool open = ImGui::TreeNodeEx(node.name.c_str(), flags);

	if (ImGui::IsItemClicked())
	{
		itemState.selectedPath = path;
	}

	// Entityのドロップを受け取り、このフォルダにプレハブとして保存する
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY"))
		{
			entt::entity droppedEntity = *(const entt::entity*)payload->Data;
			SaveEntityAsPrefab(registry, context, droppedEntity, path);
		}
		ImGui::EndDragDropTarget();
	}

	// 右クリックでコンテキストメニュー
	if (ImGui::BeginPopupContextItem(path.string().c_str()))
	{
		if (ImGui::BeginMenu("新規作成"))
		{
			if (ImGui::MenuItem("フォルダ"))
			{
				std::string newName = GenerateUniqueName(path, "New Folder");
				std::filesystem::create_directory(path / newName);
			}

			ImGui::EndMenu();
		}

		ImGui::Separator();
		if (ImGui::MenuItem("名前の変更"))
		{
			BeginRename(path);
		}

		// ルートフォルダは削除しない
		if (path != rootPath)
		{
			ImGui::Separator();
			if (ImGui::MenuItem("削除"))
			{
				itemState.mode = PrefabItemMode::ConfirmingDelete;
				itemState.targetPath = path;
			}
		}
		ImGui::EndPopup();
	}

	if (open && hasSubFolder)
	{
		for (auto& child : node.children)
		{
			DrawFolderTree(child, registry, context);
		}
		ImGui::TreePop();
	}
}
// 右：ファイル一覧
void PrefabWindow::DrawFileGrid(entt::registry& registry, EditorContext& context)
{
	if (itemState.selectedPath.empty()) return;

	const float iconSize = 64.0f;
	const float padding = 16.0f;
	const float cellSize = iconSize + padding;

	float windowWidth = ImGui::GetContentRegionAvail().x;
	int columns = (std::max)(1, (int)(windowWidth / cellSize));
	int index = 0;

	for (auto& file : fileGridCache)
	{
		const std::string& filename = file.filename;
		std::string prefabPath = file.path.string();
		std::replace(prefabPath.begin(), prefabPath.end(), '\\', '/');

		if (index % columns != 0)
		{
			ImGui::SameLine();
		}

		ImGui::BeginGroup();

		ImVec2 pos = ImGui::GetCursorScreenPos();

		// プレハブ用のアイコン（単色の四角で仮表示）
		ImGui::GetWindowDrawList()->AddRectFilled(
			pos,
			ImVec2(pos.x + iconSize, pos.y + iconSize),
			IM_COL32(90, 170, 90, 255),
			8.0f
		);
		ImGui::Dummy(ImVec2(iconSize, iconSize));

		ImGui::SetCursorScreenPos(pos);
		ImGui::PushID(index);
		ImGui::InvisibleButton("##icon", ImVec2(iconSize, iconSize));

		// 既存のプレハブに、Entityがドロップされたら、そのファイルを上書き保存する
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY"))
			{
				entt::entity droppedEntity = *(const entt::entity*)payload->Data;
				OverwritePrefab(registry, context, droppedEntity, file.path);
			}
			ImGui::EndDragDropTarget();
		}

		// ドラッグ開始（Hierarchyへドロップして配置するため）
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			ImGui::SetDragDropPayload(
				"PREFAB_PATH",
				prefabPath.c_str(),
				prefabPath.size() + 1
			);
			ImGui::Text(filename.c_str());
			ImGui::EndDragDropSource();
		}

		// 右クリックで「名前の変更」「削除」
		if (ImGui::BeginPopupContextItem("##fileContext"))
		{
			if (ImGui::MenuItem("名前の変更"))
			{
				BeginRename(file.path);
			}
			if (ImGui::MenuItem("削除"))
			{
				itemState.mode = PrefabItemMode::ConfirmingDelete;
				itemState.targetPath = file.path;
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();

		bool isRenamingThis = (itemState.mode == PrefabItemMode::Renaming && itemState.targetPath == file.path);
		if (isRenamingThis)
		{
			ImGui::SetNextItemWidth(iconSize + padding);
			ImGui::SetKeyboardFocusHere();

			bool confirmedByEnter = ImGui::InputText("##rename", itemState.editBuffer, sizeof(itemState.editBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
			bool confirmedByBlur = ImGui::IsItemDeactivated() && !confirmedByEnter;

			// 拡張子は「.prefab」の部分だけ固定表示する（.jsonの部分は内部実装として隠す）
			ImGui::TextDisabled(".prefab");

			if (confirmedByEnter || confirmedByBlur)
			{
				CommitRename();
			}
		}
		else
		{
			std::string displayName = filename;
			if (displayName.size() > 10)
				displayName = displayName.substr(0, 9) + "...";
			ImGui::TextUnformatted(displayName.c_str());
		}

		ImGui::EndGroup();
		index++;
	}

	// 空白部分の右クリックで、新規フォルダ作成
	if (ImGui::BeginPopupContextWindow("PrefabFileGridContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverExistingPopup))
	{
		if (ImGui::BeginMenu("新規作成"))
		{
			if (ImGui::MenuItem("フォルダ"))
			{
				std::string newName = GenerateUniqueName(itemState.selectedPath, "New Folder");
				std::filesystem::create_directory(itemState.selectedPath / newName);
			}
			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}
}

// Entityを、指定フォルダにプレハブとして保存する
void PrefabWindow::SaveEntityAsPrefab(entt::registry& registry, EditorContext& context, entt::entity e, const std::filesystem::path& folder)
{
	if (!registry.valid(e)) return;

	std::string name = context.GetName(e);
	std::string fileName = GenerateUniqueName(folder, name + ".prefab.json");

	nlohmann::json data = EntitySerializer::SerializeEntity(registry, e);
	EntitySerializer::SavePrefabFile((folder / fileName).string(), data);
}
// Entityを、既存のプレハブファイルに上書き保存する
void PrefabWindow::OverwritePrefab(entt::registry& registry, EditorContext& context, entt::entity e, const std::filesystem::path& filePath)
{
	if (!registry.valid(e)) return;

	nlohmann::json data = EntitySerializer::SerializeEntity(registry, e);
	EntitySerializer::SavePrefabFile(filePath.string(), data);
}
// 間隔が経過していれば、フォルダツリーのキャッシュを作り直す
void PrefabWindow::RefreshFolderTreeIfNeeded()
{
	auto now = std::chrono::steady_clock::now();
	if (now - lastFolderScanTime < folderScanInterval) return;

	folderTreeCache = BuildFolderNode(rootPath);
	lastFolderScanTime = now;
}
PrefabWindow::FolderNode PrefabWindow::BuildFolderNode(const std::filesystem::path& path)
{
	FolderNode node;
	node.path = path;
	node.name = path.filename().string();
	if (node.name.empty()) node.name = path.string();

	for (auto& entry : std::filesystem::directory_iterator(path))
	{
		if (entry.is_directory())
		{
			node.children.push_back(BuildFolderNode(entry.path()));
		}
	}
	return node;
}

// 選択フォルダが変わった／間隔が経過していれば、ファイルグリッドのキャッシュを作り直す
void PrefabWindow::RefreshFileGridIfNeeded()
{
	bool folderChanged = (itemState.selectedPath != fileGridCachedPath);
	auto now = std::chrono::steady_clock::now();
	bool timeElapsed = (now - lastFileGridScanTime >= std::chrono::seconds(2));

	if (!folderChanged && !timeElapsed) return;

	fileGridCache = BuildFileGrid(itemState.selectedPath);
	fileGridCachedPath = itemState.selectedPath;
	lastFileGridScanTime = now;
}
std::vector<PrefabWindow::FileEntry> PrefabWindow::BuildFileGrid(const std::filesystem::path& path)
{
	std::vector<FileEntry> result;

	for (auto& entry : std::filesystem::directory_iterator(path))
	{
		if (entry.is_directory()) continue;
		if (!IsPrefabFile(entry.path())) continue;

		FileEntry file;
		file.path = entry.path();
		file.filename = entry.path().filename().string();
		result.push_back(file);
	}
	return result;
}

// プレハブファイルかどうかの判定
bool PrefabWindow::IsPrefabFile(const std::filesystem::path& path)
{
	if (path.extension() != ".json") return false;

	std::string stem = path.stem().string();
	std::filesystem::path stemPath(stem);
	return stemPath.extension() == ".prefab";
}

// 削除確認ポップアップ
void PrefabWindow::DrawDeleteConfirmPopup()
{
	if (itemState.mode != PrefabItemMode::ConfirmingDelete) return;

	ImGui::OpenPopup("削除の確認");

	bool open = true;
	if (ImGui::BeginPopupModal("削除の確認", &open, ImGuiWindowFlags_AlwaysAutoResize))
	{
		std::string name = itemState.targetPath.filename().string();
		ImGui::Text("「%s」を削除しますか？", name.c_str());
		ImGui::Spacing();

		if (ImGui::Button("削除", ImVec2(120, 0)))
		{
			std::error_code ec;
			std::filesystem::remove_all(itemState.targetPath, ec);

			if (itemState.selectedPath == itemState.targetPath)
			{
				itemState.selectedPath = rootPath;
			}

			itemState.mode = PrefabItemMode::None;
			itemState.targetPath.clear();
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("キャンセル", ImVec2(120, 0)))
		{
			itemState.mode = PrefabItemMode::None;
			itemState.targetPath.clear();
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	if (!open && itemState.mode == PrefabItemMode::ConfirmingDelete)
	{
		itemState.mode = PrefabItemMode::None;
		itemState.targetPath.clear();
	}
}

// リネーム開始
void PrefabWindow::BeginRename(const std::filesystem::path& target)
{
	itemState.mode = PrefabItemMode::Renaming;
	itemState.targetPath = target;

	// プレハブファイル(.prefab.json)は、2段階の拡張子を両方取り除く。フォルダはそのまま
	std::string currentStem;
	if (IsPrefabFile(target))
	{
		currentStem = std::filesystem::path(target.stem()).stem().string(); // "xxx.prefab" -> "xxx"
	}
	else
	{
		currentStem = target.stem().string();
	}

	strncpy_s(itemState.editBuffer, currentStem.c_str(), sizeof(itemState.editBuffer) - 1);
}
// リネーム確定
void PrefabWindow::CommitRename()
{
	std::string newStem = itemState.editBuffer;

	// プレハブファイルは、常に「.prefab.json」を付け直す。フォルダはそのまま(拡張子なし)
	std::string newName = IsPrefabFile(itemState.targetPath)
		? newStem + ".prefab.json" : newStem;

	if (!newStem.empty() && newName != itemState.targetPath.filename().string())
	{
		std::filesystem::path newPath = itemState.targetPath.parent_path() / newName;

		std::error_code ec;
		std::filesystem::rename(itemState.targetPath, newPath, ec);

		if (!ec)
		{
			if (itemState.selectedPath == itemState.targetPath)
			{
				itemState.selectedPath = newPath;
			}
		}
	}

	itemState.mode = PrefabItemMode::None;
	itemState.targetPath.clear();
}