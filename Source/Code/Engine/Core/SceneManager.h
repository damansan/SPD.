#pragma once
#include "Scene.h"
#include <memory>
#include "SceneContext.h"

class SceneManager
{
public:
	SceneManager() = default;
	~SceneManager() = default;

	SceneManager(const SceneManager&) = delete;				   // コピーコンストラクタ禁止
	SceneManager& operator=(const SceneManager&) = delete;	   // コピー代入禁止
	SceneManager(SceneManager&&) noexcept = delete;			   // ムーブコンストラクタ禁止
	SceneManager& operator=(SceneManager&&) noexcept = delete; // ムーブ代入禁止


	// 更新
	void Update(float elapsedTime, const SceneContext& context);
	// 描画
	void Render(const SceneContext& context);

	// シーン切り替え予約
	void ChangeScene(std::unique_ptr<Scene>scene);

	// シーンクリア
	void ClearScene();

	// 現在のシーン取得
	Scene* GetCurrentScene() { return currentScene.get(); }

private:
	std::unique_ptr<Scene>currentScene; // 現在シーン
	std::unique_ptr<Scene>nextScene;    // 次のシーン
};