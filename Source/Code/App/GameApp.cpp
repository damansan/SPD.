#include "GameApp.h"
#include "GameApp.h"

#include "Engine/Core/Log.h"
#include "Engine/Core/JsonFileCache.h"
#include "Engine/Resource/SceneSerializer.h"
#include "Game/Scene/SceneGame.h"
#include "Game/ECS/GameComponents.h"

#include "tracy/Tracy.hpp"

#include <json.hpp>

namespace
{
	// プロジェクト設定ファイル
	const char* kProjectSettingsPath = "Data/ProjectSettings.json";
}

// 初期化
void GameApp::OnInitialize()
{
	// Projectのコンポーネントの登録(Engineのものは基盤が登録済み)
	GameComponents::Register();

	sceneManager.ChangeScene(std::make_unique<SceneGame>());

	// 最初に読み込むシーン
	startScenePath = LoadStartScenePath();

#ifndef _DEBUG
	// リリース時はボーダーレスフルスクリーンで起動
	graphics.ToggleFullscreen();
#endif
}

// シーン更新の前
void GameApp::OnPreUpdate(float elapsedTime)
{
	// ウィンドウの縦横比に合わせて、ゲームカメラのプロジェクションを更新する
	RECT rc{};
	GetClientRect(hwnd, &rc);
	float width = static_cast<float>(rc.right - rc.left);
	float height = static_cast<float>(rc.bottom - rc.top);
	if (width > 0.0f && height > 0.0f)
	{
		gameView.SetAspect(width / height);
	}
}

// シーン更新の後
void GameApp::OnPostUpdate(float elapsedTime)
{
	// SceneGameは最初のシーン更新で初期化されるので、その後に1回だけ読み込む
	if (startSceneLoaded) return;

	Scene* scene = sceneManager.GetCurrentScene();
	if (!scene) return;

	startSceneLoaded = true;

	if (startScenePath.empty()) return;

	// エンティティの名前はエディタ専用なので、受け取らない
	if (!SceneSerializer::LoadFromFile(scene->GetRegistry(), startScenePath))
	{
		Log("最初のシーンを読めない: %s", LogPath(startScenePath).c_str());
	}
}

// GameViewに描いたゲームを、画面全体に貼る
void GameApp::OnPresent()
{
	ZoneScopedN("PresentTexture");
	graphics.PresentTexture(gameView.GetOutputSRV());
}

// プロジェクト設定から、最初に読み込むシーンのパスを取得する
std::string GameApp::LoadStartScenePath()
{
	// 起動時に1回読むだけなので、その場限りで使う
	JsonFileCache<std::string> file;
	const std::string& path = file.GetOrLoad(kProjectSettingsPath,
		[](std::string& startScene, const nlohmann::json& j) { startScene = j.value("startScene", ""); });

	if (file.GetStatus(kProjectSettingsPath) == JsonFileStatus::Loaded && path.empty())
	{
		Log("プロジェクト設定に startScene が無い");
	}
	return path;
}