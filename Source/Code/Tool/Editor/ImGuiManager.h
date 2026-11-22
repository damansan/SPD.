#pragma once
#include <windows.h>
#include <d3d11.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern ImWchar glyphRangesJapanese[];


class ImGuiManager
{
public:
	ImGuiManager() = default;
	~ImGuiManager();

	ImGuiManager(const ImGuiManager&) = delete;				   // コピーコンストラクタ禁止
	ImGuiManager& operator=(const ImGuiManager&) = delete;	   // コピー代入禁止
	ImGuiManager(ImGuiManager&&) noexcept = delete;			   // ムーブコンストラクタ禁止
	ImGuiManager& operator=(ImGuiManager&&) noexcept = delete; // ムーブ代入禁止

    // 初期化
    void Initialize(HWND hWnd, ID3D11Device* device, ID3D11DeviceContext* dc);
    // フレーム開始
    void BeginFrame();
    // フレーム終了/描画
    void EndFrame();
    // 終了化
    void Shutdown();

    // ウィンドウメッセージ処理
    bool HandleMessage(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);
};