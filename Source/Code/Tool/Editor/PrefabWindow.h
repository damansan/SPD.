#pragma once
#include "EditorContext.h"
#include "entt.hpp"
#include <string>
#include <filesystem>

// フォルダ・ファイルの操作状態
enum class PrefabItemMode
{
	None,             // 何もしていない
	Renaming,         // 名前編集中
	ConfirmingDelete, // 削除確認ダイアログ表示中
};

// フォルダ・ファイル操作の状態をまとめたもの
struct PrefabItemState
{
	std::filesystem::path selectedPath;
	PrefabItemMode mode = PrefabItemMode::None;
	std::filesystem::path targetPath; // リネーム・削除対象のパス
	char editBuffer[256] = {};        // リネームの入力バッファ
};

class PrefabWindow
{
public:
	// 初期化
	void Initialize(const std::string& rootPath);

	// 描画
	void Draw(entt::registry& registry, EditorContext& context);

	// 重複しない名前で作成
	static std::string GenerateUniqueName(const std::filesystem::path& parentDir, const std::string& baseName);

private:
	// キャッシュされたフォルダツリーの1ノード
	struct FolderNode
	{
		std::filesystem::path path;
		std::string name;
		std::vector<FolderNode> children;
	};
	// キャッシュされたファイルグリッドの1エントリ
	struct FileEntry
	{
		std::filesystem::path path;
		std::string filename;
	};

	// 左：フォルダツリー
	void DrawFolderTree(FolderNode& node, entt::registry& registry, EditorContext& context);
	// 右：ファイル一覧
	void DrawFileGrid(entt::registry& registry, EditorContext& context);

	// Entityを、指定フォルダにプレハブとして保存する
	void SaveEntityAsPrefab(entt::registry& registry, EditorContext& context, entt::entity e, const std::filesystem::path& folder);
	// Entityを、既存のプレハブファイルに上書き保存する
	void OverwritePrefab(entt::registry& registry, EditorContext& context, entt::entity e, const std::filesystem::path& filePath);

	// 間隔が経過していれば、フォルダツリーのキャッシュを作り直す
	void RefreshFolderTreeIfNeeded();
	FolderNode BuildFolderNode(const std::filesystem::path& path);

	// 選択フォルダが変わった／間隔が経過していれば、ファイルグリッドのキャッシュを作り直す
	void RefreshFileGridIfNeeded();
	std::vector<FileEntry> BuildFileGrid(const std::filesystem::path& path);

	// プレハブファイルかどうかの判定
	static bool IsPrefabFile(const std::filesystem::path& path);

	// 削除確認ポップアップ
	void DrawDeleteConfirmPopup();

	// リネーム開始・確定
	void BeginRename(const std::filesystem::path& target);
	void CommitRename();

	// ルートパス
	std::filesystem::path rootPath;

	// フォルダ・ファイル操作の状態
	PrefabItemState itemState;

	// キャッシュされたフォルダツリーのルート
	FolderNode folderTreeCache;
	std::chrono::steady_clock::time_point lastFolderScanTime{};
	static constexpr std::chrono::seconds folderScanInterval{ 2 };

	// キャッシュされたファイルグリッド
	std::vector<FileEntry> fileGridCache;
	std::filesystem::path fileGridCachedPath;
	std::chrono::steady_clock::time_point lastFileGridScanTime{};
};