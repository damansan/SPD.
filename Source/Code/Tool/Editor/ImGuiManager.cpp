#include "ImGuiManager.h"

ImGuiManager::~ImGuiManager()
{
	// 終了化
	Shutdown();
}

// 初期化
void ImGuiManager::Initialize(HWND hWnd, ID3D11Device* device, ID3D11DeviceContext* dc)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();

	// ドッキング有効化
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	//マルチビューポート有効化
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	// 日本語フォント設定
	io.Fonts->AddFontFromFileTTF(
		"SystemData/Fonts/NotoSansJP-Regular.ttf",
		16.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());

	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device, dc);

	ImGui::StyleColorsClassic(); // クラシック
}

// フレーム開始
void ImGuiManager::BeginFrame()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// ドッキングスペース（画面全体）
	ImGuiWindowFlags dockFlags =
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBackground;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::Begin("DockSpace", nullptr, dockFlags);
	ImGui::PopStyleVar(3);

	// ドッキングスペースID生成
	ImGuiID dockSpaceId = ImGui::GetID("MainDockSpace");
	ImGui::DockSpace(dockSpaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

	ImGui::End();
}

// フレーム終了/描画
void ImGuiManager::EndFrame()
{
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// マルチビューポート更新
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

// 終了化
void ImGuiManager::Shutdown()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// ウィンドウメッセージ処理
bool ImGuiManager::HandleMessage(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wparam, lparam);
}
