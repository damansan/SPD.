#include "EditorApp.h"
#include "tracy/Tracy.hpp"
#include <filesystem>
#include <algorithm>

#include "Engine/ECS/Components/ControllerComponent.h"
#include "Engine/Resource/SceneSerializer.h"
#include "Game/Scene/SceneGame.h"
#include "Game/ECS/GameComponents.h"
#include "Tool/Editor/ComponentRegistryInitializer.h"
#include "Tool/Editor/FileDialog.h"
#include "GameTool/GameInspectors.h"

// 初期化
void EditorApp::OnInitialize()
{
	// ImGuiManager初期化
	imguiManager.Initialize(hwnd, graphics.GetDevice(), graphics.GetDeviceContext());

	// Viewportとデバッグカメラ
	viewportWindow.Initialize();

	// Projectのコンポーネントの登録(Engineのものは基盤が登録済み)
	GameComponents::Register();

	// インスペクターの独自描画の登録
	ComponentRegistryInitializer::Initialize();
	GameInspectors::Register();

	sceneManager.ChangeScene(std::make_unique<SceneGame>());

	// ProjectWindow初期化
	projectWindow.Initialize("Data");
	projectWindow.LoadIcons(graphics.GetDevice());
	// PrefabWindow初期化
	prefabWindow.Initialize("Data/Prefab");

	// Studio共有リソース初期化
	studioShared.Initialize(graphics.GetDevice());
	studioShared.postProcessSettings = &postProcessSettings;
}

// シーン更新の前
void EditorApp::OnPreUpdate(float elapsedTime)
{
	// F1でゲーム単体の見た目(ImGui無し)をプレビュー切り替え
	static bool prevF1 = false;
	bool currF1 = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;
	if (currF1 && !prevF1)
	{
		previewRelease = !previewRelease;
	}
	prevF1 = currF1;
	// ウィンドウの縦横比をカメラに当てる
	if (previewRelease)
	{
		RECT rc{};
		GetClientRect(hwnd, &rc);
		float width = static_cast<float>(rc.right - rc.left);
		float height = static_cast<float>(rc.bottom - rc.top);
		if (width > 0.0f && height > 0.0f)
		{
			float aspect = width / height;

			// ゲームカメラ
			gameView.SetAspect(aspect);

			// デバッグカメラ(Debugモードのままプレビューしたときも引き伸ばされないように)
			Camera& debugCamera = viewportWindow.GetDebugCamera();
			debugCamera.SetPerspectiveFov(
				DirectX::XMConvertToRadians(debugCamera.GetViewingAngle()),
				aspect, 0.1f, 1000.0f);
		}
	}


	// 右クリックを押した瞬間、Gameへの入力が有効であればDebugモードに切り替え
	static bool prevRButton = false;
	bool currRButton = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
	if (currRButton && !prevRButton && IsGameInputActive())
	{
		editorContext.cameraMode = CameraMode::Debug;
	}
	prevRButton = currRButton;
}

// シーン更新の後
void EditorApp::OnPostUpdate(float elapsedTime)
{
	// デバッグカメラ更新(シーンの後に行う)
	UpdateDebugCamera(elapsedTime);

	// 移動の記録
	if (auto* scene = sceneManager.GetCurrentScene())
	{
		movementMonitor.Record(scene->GetRegistry(), elapsedTime);
	}

	// Studio更新
	for (auto& win : studioWindows)
	{
		win->Update(elapsedTime);
	}
}

// 毎フレーム1回(ImGui)
void EditorApp::OnFrame()
{
	ZoneScoped;

	// プレビュー中はImGui自体を一切使わない
	if (!IsEditorActive()) return;

	// ImGuiフレーム開始
	imguiManager.BeginFrame();

	// Gameウィンドウ(Viewportとドッキングレイアウト)
	viewportWindow.Draw(gameView, [this]() { DrawMainMenu(); });

	if (auto* scene = sceneManager.GetCurrentScene())
	{
		projectWindow.Draw(scene->GetRegistry(), editorContext);
		hierarchyWindow.Draw(scene->GetRegistry(), editorContext);
		inspectorWindow.Draw(scene->GetRegistry(), editorContext);
		prefabWindow.Draw(scene->GetRegistry(), editorContext);
		adaptiveTriggerTool.Draw(scene->GetRegistry());
	}
	// シーンが無くても、ログは見られるようにする
	logWindow.Draw();
	// 実行中に確認するツール
	movementMonitor.Draw();
	postProcessTool.Draw(postProcessSettings);

	// StudioOpenRequestを処理
	if (editorContext.studioRequest.pending)
	{
		OpenStudioWindow(editorContext.studioRequest.assetPath);
		editorContext.studioRequest.pending = false;
		editorContext.studioRequest.assetPath = "";
	}
	for (auto& win : studioWindows)
	{
		std::string title = "Studio##" + std::to_string(win->GetWindowId());
		win->DrawWindow(title.c_str());
	}
	// 閉じるリクエストが出たウィンドウを削除
	studioWindows.erase(
		std::remove_if(studioWindows.begin(), studioWindows.end(),
			[](const std::unique_ptr<StudioWindow>& w)
			{
				return w->IsCloseRequested();
			}),
		studioWindows.end()
	);

	// 全てのウィンドウの描画後にシーン保存・読み込みを実行する
	ProcessSceneRequest();

	// Openの確認ポップアップ
	if (ImGui::BeginPopupModal("Open Scene##Confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Current scene will be discarded.\nAre you sure you want to continue?");
		ImGui::Spacing();

		if (ImGui::Button("Open", ImVec2(120, 0)))
		{
			Scene* scene = sceneManager.GetCurrentScene();
			if (scene)
			{
				// 破棄する古いEntityへの参照が残らないよう、読み込みの前に、選択・名前マップをリセットする
				editorContext.Deselect();
				editorContext.entityNames.clear();

				auto setName = [this](entt::entity e, const std::string& name) { editorContext.SetName(e, name); };

				if (SceneSerializer::LoadFromFile(scene->GetRegistry(), pendingOpenPath, setName))
				{
					editorContext.currentScenePath = pendingOpenPath;
				}
			}
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

// GameViewに描いたゲームを、画面へ出す
void EditorApp::OnPresent()
{
	if (IsEditorActive())
	{
		// Studio描画
		{
			ZoneScopedN("Studio::Render");
			for (auto& win : studioWindows)
			{
				win->Render();
			}
		}

		// Studioが描画先を変えるので、メインRTVを再セット
		graphics.SetRenderTargets();

		// ImGui描画(GameViewのテクスチャも、ここでViewport経由で表示される)
		{
			ZoneScopedN("ImGui::EndFrame");
			imguiManager.EndFrame();
		}
	}
	else
	{
		// プレビュー中：ゲームのテクスチャを画面全体に貼る
		ZoneScopedN("PresentTexture");
		graphics.PresentTexture(gameView.GetOutputSRV());
	}
}

// メッセージ
bool EditorApp::OnMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// ImGuiメッセージ処理
	return imguiManager.HandleMessage(hwnd, msg, wparam, lparam);
}

// 描画に使うカメラ(Debugならデバッグカメラ、Gameならゲームカメラ)
Camera& EditorApp::GetRenderCamera()
{
	return (editorContext.cameraMode == CameraMode::Debug)
		? viewportWindow.GetDebugCamera()
		: gameView.GetCamera();
}

// Gameへの入力を受け付けてよいか(エディタ表示中はViewportにマウスがあるときだけ)
bool EditorApp::IsGameInputActive() const
{
	return IsEditorActive() ? viewportWindow.IsViewportHovered() : true;
}

// エディタのUIを表示しているか
bool EditorApp::IsEditorActive() const
{
	return !previewRelease;
}

// デバッグカメラの更新と、Debug/Gameの切り替え
void EditorApp::UpdateDebugCamera(float elapsedTime)
{
	if (editorContext.cameraMode != CameraMode::Debug) return;

	Scene* scene = sceneManager.GetCurrentScene();
	if (!scene) return;
	entt::registry& registry = scene->GetRegistry();

	debugCameraSystem.Update(registry, elapsedTime, IsGameInputActive());
	debugCameraSystem.ApplyToCamera(registry, viewportWindow.GetDebugCamera());

	// Gameへの入力が有効でなければ、切り替えは行わない
	if (!IsGameInputActive()) return;

	// スティック入力があればGameモードに戻す
	// (Projectの入力ではなく、Engineのコントローラーの生の値を見る)
	const float threshold = 0.2f;
	auto view = registry.view<ControllerComponent>();
	for (auto entity : view)
	{
		const auto& ctrl = view.get<ControllerComponent>(entity);
		if (fabsf(ctrl.leftStickX) > threshold ||
			fabsf(ctrl.leftStickY) > threshold ||
			fabsf(ctrl.rightStickX) > threshold ||
			fabsf(ctrl.rightStickY) > threshold)
		{
			editorContext.cameraMode = CameraMode::Game;
			break;
		}
	}
}

// Studioを開く
void EditorApp::OpenStudioWindow(const std::string& assetPath)
{
	auto win = std::make_unique<StudioWindow>();
	win->Initialize(graphics.GetDevice(), &studioShared);
	win->GetContext().deviceContext = graphics.GetDeviceContext();
	win->SetWindowId(nextStudioWindowId++);

	// パスが指定されていたらモデルをロード
	if (!assetPath.empty())
	{
		win->LoadAsset(assetPath);
	}

	studioWindows.push_back(std::move(win));
}

// メニューバーの中身
void EditorApp::DrawMainMenu()
{
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("Save"))
		{
			editorContext.sceneRequest = SceneAction::Save;
		}
		if (ImGui::MenuItem("Save As..."))
		{
			editorContext.sceneRequest = SceneAction::SaveAs;
		}
		if (ImGui::MenuItem("Open Scene..."))
		{
			editorContext.sceneRequest = SceneAction::Open;
		}

		ImGui::Separator();

		if (ImGui::BeginMenu("System"))
		{
			ImGui::MenuItem("VSync", nullptr, &vsync);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Tool"))
		{
			ImGui::MenuItem("Movement Monitor", nullptr, &movementMonitor.isOpen);
			ImGui::MenuItem("Post Process", nullptr, &postProcessTool.isOpen);
			ImGui::MenuItem("Adaptive Trigger Tool", nullptr, &adaptiveTriggerTool.isOpen);
			ImGui::EndMenu();
		}

		ImGui::EndMenu();
	}

	// 今開いているシーン（Saveの上書き先）
	std::string sceneName = editorContext.currentScenePath.empty()
		? "(unsaved)"
		: std::filesystem::path(editorContext.currentScenePath).filename().string();
	ImGui::TextDisabled("Scene: %s", sceneName.c_str());
}

// シーンの保存・読み込みリクエストを実行する
void EditorApp::ProcessSceneRequest()
{
	SceneAction action = editorContext.sceneRequest;
	if (action == SceneAction::None) return;
	editorContext.sceneRequest = SceneAction::None;

	Scene* scene = sceneManager.GetCurrentScene();
	if (!scene) return;

	if (action == SceneAction::Open)
	{
		timer.stop();
		std::string path;
		bool selected = FileDialog::OpenScene(hwnd, path);
		timer.start();

		if (selected)
		{
			// 既存のEntityを破棄するため、確認ポップアップを次のフレームで開く
			pendingOpenPath = path;
			ImGui::OpenPopup("Open Scene##Confirm");
		}
		return;
	}

	std::string path = editorContext.currentScenePath;

	// Save As、またはまだ保存先が無いSaveは、ダイアログで保存先を選ぶ
	if (action == SceneAction::SaveAs || path.empty())
	{
		timer.stop();
		bool selected = FileDialog::SaveScene(hwnd, path);
		timer.start();

		if (!selected) return; // キャンセル
	}

	auto getName = [this](entt::entity e) { return editorContext.GetName(e); };
	if (SceneSerializer::SaveToFile(scene->GetRegistry(), path, getName))
	{
		editorContext.currentScenePath = path;
	}
}