#include "SceneManager.h"

// 更新
void SceneManager::Update(float elapsedTime, const SceneContext& context)
{
	// シーン切り替え処理
	if (nextScene)
	{
		// 現在シーン終了化
		if (currentScene)
		{
			currentScene->Finalize();
		}

		// シーン切り替え
		currentScene = std::move(nextScene);
		nextScene = nullptr;

		// コンテキストを渡して初期化
		currentScene->Initialize(context);
	}

	if (currentScene)
	{
		currentScene->Update(elapsedTime, context);
	}
}

// 描画
void SceneManager::Render(const SceneContext& context)
{
	if (currentScene)
	{
		currentScene->Render(context);
	}
}

// シーン切り替え予約
void SceneManager::ChangeScene(std::unique_ptr<Scene> scene)
{
	// 次フレームで切り替え
	nextScene = std::move(scene);
}

// シーンクリア
void SceneManager::ClearScene()
{
	if (currentScene)
	{
		currentScene->Finalize();
		currentScene.reset();
	}
	nextScene.reset();
}
