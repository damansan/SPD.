#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <d3d11.h>
#include <wrl.h>

#include "EditorContext.h"
#include "entt.hpp"

// フォルダ・ファイルの操作状態
enum class ProjectItemMode
{
    None,            // 何もしていない
    Renaming,        // 名前編集中
    ConfirmingDelete,// 削除確認ダイアログ表示中
};

// フォルダ・ファイルの操作状態まとめ
struct ProjectItemState
{
    std::filesystem::path selectedPath;   // 選択中のファイル・フォルダ
    ProjectItemMode        mode = ProjectItemMode::None;
    std::filesystem::path  targetPath;           // リネーム・削除対象のパス
    char                   editBuffer[256] = {}; // リネーム中の入力バッファ
};


class ProjectWindow
{
public:
    // 初期化（走査するルートパスを設定）
    void Initialize(const std::string& rootPath);
    // アイコン読み込み
    void LoadIcons(ID3D11Device* device);
    // 描画
    void Draw(entt::registry& registry, EditorContext& context, bool asDrawer = false);

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
        std::string filename; // 拡張子を除いたファイル名（表示用）
        bool isModel = false;
        bool isStateGraph = false;
        bool isSequencer = false;
        bool isAnimSpeed = false;
    };


    // 左側：フォルダツリー
    void DrawFolderTree(FolderNode& node);
    // 右側：ファイル一覧
    void DrawFileGrid(entt::registry& registry, EditorContext& context);

    // 一定間隔が経過していれば、フォルダツリーのキャッシュを作り直す
    void RefreshFolderTreeIfNeeded();
    // ディスクを実際に走査して、指定パス以下のFolderNodeを構築する（再帰）
    FolderNode BuildFolderNode(const std::filesystem::path& path);

    // 選択フォルダが変わった／一定間隔が経過していれば、ファイルグリッドのキャッシュを作り直す
    void RefreshFileGridIfNeeded();
    // ディスクを実際に走査して、指定フォルダ直下のFileEntry一覧を構築する
    std::vector<FileEntry> BuildFileGrid(const std::filesystem::path& path);

    // ファイルがモデルファイルか判定
    bool IsModelFile(const std::filesystem::path& path);
    // ファイルがステートグラフファイルか判定
    bool IsStateGraphFile(const std::filesystem::path& path);
	// ファイルがシーケンサーファイルか判定
    static bool IsSequencerFile(const std::filesystem::path& path);
    // ファイルがアニメーション速度ファイルか判定
    static bool IsAnimSpeedFile(const std::filesystem::path& path);

    // 削除確認ポップ
    void DrawDeleteConfirmPopup();

    // リネーム開始
    void BeginRename(const std::filesystem::path& target);
    // リネーム確定
    void CommitRename();

    // ルートパス
    std::filesystem::path rootPath;

    // フォルダ・ファイル共通の操作状態
    ProjectItemState itemState;

    // キャッシュされたフォルダツリーのルート
    FolderNode folderTreeCache;

    // フォルダツリー再スキャンの間隔管理
    std::chrono::steady_clock::time_point lastFolderScanTime{};
    static constexpr std::chrono::seconds folderScanInterval{ 2 };

    // キャッシュされたファイルグリッド
    std::vector<FileEntry> fileGridCache;
    // fileGridCacheがどのフォルダに対するキャッシュかを記録
    std::filesystem::path fileGridCachedPath;
    // ファイルグリッド再スキャンの間隔管理
    std::chrono::steady_clock::time_point lastFileGridScanTime{};

    // モデルアイコン用SRV
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> modelIconSRV;
};