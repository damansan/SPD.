#include "ProjectWindow.h"
#include <imgui.h>

#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Components/MeshComponent.h"
#include <algorithm>
#include <fstream>
#include <WICTextureLoader.h>
#include <json.hpp>

#include "tracy/Tracy.hpp"

void ProjectWindow::Initialize(const std::string& rootPath)
{
	this->rootPath = rootPath;
	// 初期選択フォルダはルート
	itemState.selectedPath = rootPath;
}

// アイコン読み込み
void ProjectWindow::LoadIcons(ID3D11Device* device)
{
	// Data/Sprite/System/folder.png を読み込む
	DirectX::CreateWICTextureFromFile(
		device,
		L"SystemData/Sprite/folder.png",
		nullptr,
		modelIconSRV.GetAddressOf()
	);
}

void ProjectWindow::Draw(entt::registry& registry, EditorContext& context, bool asDrawer)
{
	ZoneScoped;

	// 2秒経過していればディスクを見てキャッシュを作り直す
	RefreshFolderTreeIfNeeded();
	RefreshFileGridIfNeeded();

	ImGui::Begin("Project");

	if (ImGui::BeginTable("ProjectLayout", 2,
		ImGuiTableFlags_Resizable |
		ImGuiTableFlags_BordersInnerV))
	{
		// 左側の初期幅を設定
		ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, 200.0f);
		ImGui::TableSetupColumn("Files", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();

		// 左側：フォルダツリー
		ImGui::TableSetColumnIndex(0);
		ImGui::BeginChild("FolderTree", ImVec2(0, 0), false);
		DrawFolderTree(folderTreeCache);
		ImGui::EndChild();

		// 右側：ファイルグリッド
		ImGui::TableSetColumnIndex(1);
		ImGui::BeginChild("FileGrid", ImVec2(0, 0), false);
		DrawFileGrid(registry, context);
		ImGui::EndChild();

		ImGui::EndTable();
	}

	DrawDeleteConfirmPopup();

	ImGui::End();
}

// 重複しない名前で作成
std::string ProjectWindow::GenerateUniqueName(const std::filesystem::path& parentDir, const std::string& baseName) 
{
	std::filesystem::path candidate = parentDir / baseName;
	if (!std::filesystem::exists(candidate))
	{
		return baseName;
	}

	// 拡張子の手前に連番を挟む
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

// フォルダツリー
void ProjectWindow::DrawFolderTree(FolderNode& node)
{
	const std::filesystem::path& path = node.path;
	bool isRenamingThis = (itemState.mode == ProjectItemMode::Renaming && itemState.targetPath == path);

	// キャッシュを確認してサブフォルダを確認する
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

	// 選択中フォルダをハイライト
	if (itemState.selectedPath == path)
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	// 最下層の場合は葉ノードとして表示
	if (!hasSubFolder)
	{
		flags |= ImGuiTreeNodeFlags_Leaf;
		flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	bool open = ImGui::TreeNodeEx(node.name.c_str(), flags);

	// クリックで選択
	if (ImGui::IsItemClicked())
	{
		itemState.selectedPath = path;
	}

	// 右クリックでコンテキストメニュー（このフォルダの中に作る）
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

		// ルートフォルダは削除させない
		if (path != rootPath)
		{
			ImGui::Separator();
			if (ImGui::MenuItem("削除"))
			{
				itemState.mode = ProjectItemMode::ConfirmingDelete;
				itemState.targetPath = path;
			}
		}
		ImGui::EndPopup();
	}

	// 最下層はTreePopが不要
	if (open && hasSubFolder)
	{
		for (auto& child : node.children)
		{
			DrawFolderTree(child);
		}
		ImGui::TreePop();
	}

}
// ファイル一覧
void ProjectWindow::DrawFileGrid(entt::registry& registry, EditorContext& context)
{
	if (itemState.selectedPath.empty()) return;

	// アイコンサイズと余白
	const float iconSize = 64.0f;
	const float padding = 16.0f;
	const float cellSize = iconSize + padding;

	// ウィンドウ幅から列数を計算
	float windowWidth = ImGui::GetContentRegionAvail().x;
	int   columns = (std::max)(1, (int)(windowWidth / cellSize));

	int index = 0;

	for (auto& file : fileGridCache)
	{
		bool isModel = file.isModel;
		bool isAnimGraph = file.isStateGraph;
		bool isSequencer = file.isSequencer;
		bool isAnimSpeed = file.isAnimSpeed;

		const std::string& filename = file.filename;
		std::string modelPath = file.path.string();
		std::replace(modelPath.begin(), modelPath.end(), '\\', '/');

		// 列の折り返し
		if (index % columns != 0)
		{
			ImGui::SameLine();
		}

		// グループ化してアイコン＋名前をまとめる
		ImGui::BeginGroup();

		ImVec2 pos = ImGui::GetCursorScreenPos();

		// アイコン表示（モデル用アイコンが無い/モデル以外は色付き矩形で代用）
		if (isModel && modelIconSRV)
		{
			ImGui::Image(
				(ImTextureID)modelIconSRV.Get(),
				ImVec2(iconSize, iconSize)
			);
		}
		else
		{
			ImU32 color = isAnimGraph ? IM_COL32(200, 130, 60, 255)
				: isSequencer ? IM_COL32(120, 200, 130, 255)
				: isAnimSpeed ? IM_COL32(200, 90, 160, 255)
				: IM_COL32(80, 120, 200, 255);
			ImGui::GetWindowDrawList()->AddRectFilled(
				pos,
				ImVec2(pos.x + iconSize, pos.y + iconSize),
				color,
				8.0f
			);
			ImGui::Dummy(ImVec2(iconSize, iconSize));
		}

		// 画像の上に透明ボタンを重ねてドラッグを取る
		ImGui::SetCursorScreenPos(pos);
		ImGui::PushID(index);
		ImGui::InvisibleButton("##icon", ImVec2(iconSize, iconSize));

		// ダブルクリックでStudioを開くリクエスト（モデル／アニメーショングラフ）
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
		{
			if (isModel)
			{
				context.studioRequest.pending = true;
				context.studioRequest.assetPath = modelPath;
			}
		}

		// ドラッグ開始（モデル／シーケンサー/アニメーション速度）
		if ((isModel || isSequencer || isAnimSpeed) && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			if (isModel)
			{
				ImGui::SetDragDropPayload(
					"MODEL_PATH",
					modelPath.c_str(),
					modelPath.size() + 1
				);
			}
			else if (isSequencer)
			{
				ImGui::SetDragDropPayload(
					"SEQUENCER_PATH",
					modelPath.c_str(),
					modelPath.size() + 1
				);
			}
			else // isAnimSpeed
			{
				ImGui::SetDragDropPayload(
					"ANIMSPEED_PATH",
					modelPath.c_str(),
					modelPath.size() + 1
				);
			}

			if (isModel && modelIconSRV)
			{
				ImGui::Image(
					(ImTextureID)modelIconSRV.Get(),
					ImVec2(iconSize, iconSize)
				);
			}
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
				itemState.mode = ProjectItemMode::ConfirmingDelete;
				itemState.targetPath = file.path;
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();

		// ファイル名表示 or リネーム中はインライン入力欄
		bool isRenamingThis = (itemState.mode == ProjectItemMode::Renaming && itemState.targetPath == file.path);
		if (isRenamingThis)
		{
			std::string extension = file.path.extension().string();

			// 入力欄はアイコン幅いっぱいに使う（拡張子は次の行に表示するため幅を分ける必要が無い）
			ImGui::SetNextItemWidth(iconSize + padding);
			ImGui::SetKeyboardFocusHere();

			bool confirmedByEnter = ImGui::InputText("##rename", itemState.editBuffer, sizeof(itemState.editBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
			bool confirmedByBlur = ImGui::IsItemDeactivated() && !confirmedByEnter;

			// 拡張子は入力欄の下の行に固定表示（編集不可）
			if (!extension.empty())
			{
				ImGui::TextDisabled("%s", extension.c_str());
			}

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

	// 空白部分の右クリックで新規作成メニュー
	if (ImGui::BeginPopupContextWindow("FileGridContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverExistingPopup))
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

// 一定間隔が経過していれば、フォルダツリーのキャッシュを作り直す(2秒)
void ProjectWindow::RefreshFolderTreeIfNeeded()
{
	auto now = std::chrono::steady_clock::now();
	if (now - lastFolderScanTime < folderScanInterval) return;

	folderTreeCache = BuildFolderNode(rootPath);
	lastFolderScanTime = now;
}
// ディスクを実際に走査して、指定パス以下のFolderNodeを構築する（再帰）
ProjectWindow::FolderNode ProjectWindow::BuildFolderNode(const std::filesystem::path& path)
{
	FolderNode node;
	node.path = path;
	node.name = path.filename().string();

	if (!std::filesystem::is_directory(path)) return node;

	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		if (entry.is_directory())
		{
			node.children.push_back(BuildFolderNode(entry.path()));
		}
	}

	return node;
}

// 選択フォルダが変わった／一定間隔が経過していれば、ファイルグリッドのキャッシュを作り直す
void ProjectWindow::RefreshFileGridIfNeeded()
{
	// 選択フォルダが前回と違う場合は、待たずに即座に作り直す
	bool folderChanged = (itemState.selectedPath != fileGridCachedPath);

	auto now = std::chrono::steady_clock::now();
	bool intervalElapsed = (now - lastFileGridScanTime >= folderScanInterval);

	if (!folderChanged && !intervalElapsed) return;

	if (itemState.selectedPath.empty() || !std::filesystem::exists(itemState.selectedPath))
	{
		fileGridCache.clear();
	}
	else
	{
		fileGridCache = BuildFileGrid(itemState.selectedPath);
	}

	fileGridCachedPath = itemState.selectedPath;
	lastFileGridScanTime = now;
}
// ディスクを実際に走査して、指定フォルダ直下のFileEntry一覧を構築する
std::vector<ProjectWindow::FileEntry> ProjectWindow::BuildFileGrid(const std::filesystem::path& path)
{
	std::vector<FileEntry> result;

	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		if (!entry.is_regular_file()) continue;

		bool isModel = IsModelFile(entry.path());
		bool isAnimGraph = IsStateGraphFile(entry.path());
		bool isSequencer = IsSequencerFile(entry.path());
		bool isAnimSpeed = IsAnimSpeedFile(entry.path());
		if (!isModel && !isAnimGraph && !isSequencer && !isAnimSpeed) continue;

		FileEntry file;
		file.path = entry.path();
		file.filename = entry.path().stem().string();
		file.isModel = isModel;
		file.isStateGraph = isAnimGraph;
		file.isSequencer = isSequencer;
		file.isAnimSpeed = isAnimSpeed;

		result.push_back(std::move(file));
	}

	return result;
}

// ファイルがモデルファイルか判定
bool ProjectWindow::IsModelFile(const std::filesystem::path& path)
{
	std::string ext = path.extension().string();

	// 小文字に変換して比較
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	return ext == ".gltf" || ext == ".glb";
}
// ファイルがアニメーショングラフファイルか判定
bool ProjectWindow::IsStateGraphFile(const std::filesystem::path& path)
{
	std::string ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	return ext == ".stategraph";
}
// ファイルがシーケンサーファイルか判定
bool ProjectWindow::IsSequencerFile(const std::filesystem::path& path)
{
	std::string ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	return ext == ".sequencer";
}

// ファイルがアニメーション速度ファイルか判定
bool ProjectWindow::IsAnimSpeedFile(const std::filesystem::path& path)
{
	std::string ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	return ext == ".animspeed";
}


// 削除確認ポップ
void ProjectWindow::DrawDeleteConfirmPopup()
{
	if (itemState.mode != ProjectItemMode::ConfirmingDelete) return;

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

			itemState.mode = ProjectItemMode::None;
			itemState.targetPath.clear();
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("キャンセル", ImVec2(120, 0)))
		{
			itemState.mode = ProjectItemMode::None;
			itemState.targetPath.clear();
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	// ×ボタンで閉じられた場合も状態をリセット
	if (!open && itemState.mode == ProjectItemMode::ConfirmingDelete)
	{
		itemState.mode = ProjectItemMode::None;
		itemState.targetPath.clear();
	}
}

// リネーム開始
void ProjectWindow::BeginRename(const std::filesystem::path& target)
{
	itemState.mode = ProjectItemMode::Renaming;
	itemState.targetPath = target;

	std::string currentStem = target.stem().string();
	strncpy_s(itemState.editBuffer, currentStem.c_str(), sizeof(itemState.editBuffer) - 1);
}
// リネーム確定
void ProjectWindow::CommitRename()
{
	std::string newStem = itemState.editBuffer;
	std::string extension = itemState.targetPath.extension().string(); // 拡張子は固定（変更不可）
	std::string newName = newStem + extension;

	// 空欄や変更なしの場合は何もせず終了
	if (!newStem.empty() && newName != itemState.targetPath.filename().string())
	{
		std::filesystem::path newPath = itemState.targetPath.parent_path() / newName;

		std::error_code ec;
		std::filesystem::rename(itemState.targetPath, newPath, ec);

		if (!ec)
		{
			// 選択中パスがリネーム対象だった場合、新しいパスに追従させる
			if (itemState.selectedPath == itemState.targetPath)
			{
				itemState.selectedPath = newPath;
			}
		}
	}

	itemState.mode = ProjectItemMode::None;
	itemState.targetPath.clear();
}
