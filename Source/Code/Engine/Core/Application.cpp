#include "Application.h"
#include "tracy/Tracy.hpp"
#include "Engine/ECS/EngineComponents.h"
#include <sstream>

Application::Application(HWND hwnd) : hwnd(hwnd)
{
}

Application::~Application()
{
	// PhysX終了処理
	physX.Cleanup();

	// Graphics終了処理
	graphics.Cleanup();
}

// 初期化
void Application::Initialize()
{
	// Graphics初期化
	graphics.Initialize(hwnd);

	// PhysX初期化
	physX.Initialize();

	// ゲームの描画先とゲームカメラ
	gameView.Initialize(graphics.GetDevice());

	// Engineのコンポーネントの登録
	EngineComponents::Register();

	// 派生クラスの初期化
	OnInitialize();

	UpdateSceneContext();
}

// アプリケーションループ
int Application::Run()
{
	MSG msg{};

	// 固定タイムステップ設定(60fps)
	const double fixedTimestep = 1.0 / 60.0;
	double accumulator = 0.0;

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			timer.tick();
			CalculateFrameStats();

			// 経過時間を蓄積
			double frameTime = (double)timer.time_interval();
			if (frameTime > 0.25) frameTime = 0.25;
			accumulator += frameTime;

			// 固定タイムステップでUpdateを実行
			while (accumulator >= fixedTimestep)
			{
				Update((float)fixedTimestep);
				accumulator -= fixedTimestep;
				updateCountBuf++;
			}

			// 毎フレーム1回
			OnFrame();

			// 描画は毎フレーム
			Render();
		}
	}

	// 終了処理(派生クラス → シーンの順)
	OnFinalize();
	sceneManager.ClearScene();

	return static_cast<int>(msg.wParam);
}

// 更新
void Application::Update(float elapsedTime)
{
	ZoneScoped;

	OnPreUpdate(elapsedTime);

	// SceneContext更新
	UpdateSceneContext();

	// シーン更新
	sceneManager.Update(elapsedTime, sceneContext);

	OnPostUpdate(elapsedTime);

	// 描画に使うカメラの結果をcontextに反映
	Camera& renderCamera = GetRenderCamera();
	sceneContext.renderCamera = &renderCamera;
	sceneContext.view = renderCamera.GetView();
	sceneContext.projection = renderCamera.GetProjection();
}

// 描画
void Application::Render()
{
	ZoneScoped;

	// 画面クリア
	graphics.Clear(0.2f, 0.2f, 0.5f, 1.0f);
	graphics.SetRenderTargets();

	// ゲームをGameViewのテクスチャに描く
	gameView.Begin(graphics.GetDeviceContext());
	sceneManager.Render(sceneContext);
	{
		ZoneScopedN("GameView::End");
		gameView.End(graphics.GetDeviceContext());
	}

	// メインRTVを再セットしてから、画面へ出す
	graphics.SetRenderTargets();
	OnPresent();

	// 画面反映
	{
		ZoneScopedN("Present");
		graphics.Present(vsync);
	}

	FrameMark;
}

// SceneContextを最新状態に更新する
void Application::UpdateSceneContext()
{
	sceneContext.device = graphics.GetDevice();
	sceneContext.deviceContext = graphics.GetDeviceContext();
	sceneContext.resourceManager = &resourceManager;
	sceneContext.physX = &physX;

	// GameViewの専用テクスチャを描画先として渡す
	sceneContext.renderTargetView = gameView.GetSceneRTV();
	sceneContext.sceneShaderResourceView = gameView.GetSceneSRV();
	sceneContext.outputRenderTargetView = gameView.GetOutputRTV();
	sceneContext.depthStencilView = gameView.GetDSV();
	sceneContext.postProcessSettings = &postProcessSettings;
	sceneContext.camera = &gameView.GetCamera();

	Camera& renderCamera = GetRenderCamera();
	sceneContext.renderCamera = &renderCamera;
	sceneContext.view = renderCamera.GetView();
	sceneContext.projection = renderCamera.GetProjection();

	sceneContext.screenWidth = gameView.GetRenderWidth();
	sceneContext.screenHeight = gameView.GetRenderHeight();
	sceneContext.gameInputActive = IsGameInputActive();
	sceneContext.debugDrawAllowed = IsDebugDrawAllowed();
}

// フレーム計算
void Application::CalculateFrameStats()
{
	static int frames = 0;
	static float timeElapsed = 0.0f;

	frames++;

	if ((timer.time_stamp() - timeElapsed) >= 1.0f)
	{
		float fps = static_cast<float>(frames);
		float mspf = 1000.0f / fps;

		updateCount = updateCountBuf;
		updateCountBuf = 0;

		std::ostringstream outs;
		outs.precision(6);
		outs << "FPS : " << fps
			<< " / Frame Time : " << mspf << " ms"
			<< " / Update : " << updateCount << " fps";
		SetWindowTextA(hwnd, outs.str().c_str());

		frames = 0;
		timeElapsed += 1.0f;
	}
}

// メッセージハンドラ
LRESULT Application::HandleMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// 派生クラスが処理したものは、ここで終わり
	if (OnMessage(hwnd, msg, wparam, lparam)) { return true; }

	switch (msg)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps{};
		BeginPaint(hwnd, &ps);
		EndPaint(hwnd, &ps);
	}
	break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CREATE:
		break;
	case WM_KEYDOWN:
		if (wparam == VK_ESCAPE)
		{
			PostMessage(hwnd, WM_CLOSE, 0, 0);
		}
		break;
	case WM_ENTERSIZEMOVE:
		timer.stop();
		break;
	case WM_SIZE:
		if (wparam == SIZE_MINIMIZED)
		{
			// 最小化時はタイマー停止のみ(リサイズ不要)
			timer.stop();
		}
		else
		{
			// 最大化/復元/通常リサイズ時
			timer.start();
			graphics.ResizeBuffers();
			UpdateSceneContext();
		}
		break;
	case WM_EXITSIZEMOVE:
		timer.start();
		// ウィンドウサイズ変更時にバッファ再生成
		graphics.ResizeBuffers();
		UpdateSceneContext();
		break;
	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	return 0;
}