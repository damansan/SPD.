#pragma once
#include <windows.h>
#include "Engine/Utility/high_resolution_timer.h"
#include "Engine/Core/Graphics.h"
#include "Engine/Core/SceneManager.h"
#include "Engine/Core/SceneContext.h"
#include "Engine/Core/GameView.h"
#include "Engine/Core/PhysX.h"
#include "Engine/Resource/ResourceManager.h"

// アプリケーションの基盤
// 毎フレームの流れを決め、派生クラス(EditorApp / GameApp)が中身を足す入り口を持つ
class Application
{
public:
	explicit Application(HWND hwnd);
	virtual ~Application();

	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;
	Application(Application&&) noexcept = delete;
	Application& operator=(Application&&) noexcept = delete;

	// 初期化
	void Initialize();

	// アプリケーションループp
	int Run();

	// メッセージハンドラ
	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

protected:
	// ===== 派生クラスが中身を足す入り口 =====
	virtual void OnInitialize() {}
	virtual void OnFinalize() {}

	// シーン更新の前・後(固定タイムステップごと)
	virtual void OnPreUpdate(float elapsedTime) {}
	virtual void OnPostUpdate(float elapsedTime) {}

	// 毎フレーム1回(ImGuiなど)
	virtual void OnFrame() {}

	// GameViewに描いたゲームを、画面へ出す
	virtual void OnPresent() = 0;

	// ポストエフェクトの設定
	PostProcessSettings postProcessSettings;

	// メッセージ(処理した場合はtrueを返す)
	virtual bool OnMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) { return false; }

	// 描画に使うカメラ(既定はゲームカメラ)
	virtual Camera& GetRenderCamera() { return gameView.GetCamera(); }

	// Gameへの入力を受け付けてよいか(既定は常に受け付ける)
	virtual bool IsGameInputActive() const { return true; }
	// デバッグ描画を許可するか(既定は不許可。エディタだけ許可する)
	virtual bool IsDebugDrawAllowed() const { return false; }

	// SceneContextを最新状態に更新する
	void UpdateSceneContext();

	// ===== 基盤のサービス =====
	HWND hwnd;
	high_resolution_timer timer;
	Graphics graphics;
	PhysX physX;
	ResourceManager resourceManager;
	SceneManager sceneManager;
	GameView gameView;
	SceneContext sceneContext;

	bool vsync = true;

private:
	void Update(float elapsedTime);
	void Render();
	void CalculateFrameStats();

	int updateCount = 0;
	int updateCountBuf = 0;
};