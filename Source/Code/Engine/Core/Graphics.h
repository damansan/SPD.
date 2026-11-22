#pragma once
#include <d3d11.h>
#include <wrl.h>


class Graphics
{
public:
	Graphics() = default;
	~Graphics() = default;

	Graphics(const Graphics&) = delete;                // コピーコンストラクタ禁止
	Graphics& operator=(const Graphics&) = delete;     // コピー代入禁止
	Graphics(Graphics&&) noexcept = delete;            // ムーブコンストラクタ禁止
	Graphics& operator=(Graphics&&) noexcept = delete; // ムーブ代入禁止

	// 初期化
	void Initialize(HWND hWnd);
	// 画面クリア
	void Clear(float r, float g, float b, float a);
	// レンダーターゲット設定
	void SetRenderTargets();
	// 指定テクスチャをフルスクリーンでバックバッファへ描画
	void PresentTexture(ID3D11ShaderResourceView* srv);
	// 画面プレゼント
	void Present(bool vsync = false);
	// フルスクリーン切り替え
	void ToggleFullscreen();
	// バッファ再生成
	void ResizeBuffers();

	// 終了処理
	void Cleanup();

	// 画面リサイズ設定
	void SetResizing(bool value) { isResizing = value; }
	// 画面リサイズ中か取得
	bool GetResizing() const { return isResizing; }

	// 画面幅取得
	float GetScreenWidth() const { return screenWidth; }
	// 画面高さ取得
	float GetScreenHeight() const { return screenHeight; }

	// ウィンドウハンドル取得
	HWND GetWindowHandle() const { return hWnd; }

	// デバイス取得
	ID3D11Device* GetDevice() { return device.Get(); }
	// デバイスコンテキスト取得
	ID3D11DeviceContext* GetDeviceContext() { return immediateContext.Get(); }

	// Graphics.h に追加
	ID3D11RenderTargetView* GetRenderTargetView() { return renderTargetView.Get(); }
	ID3D11DepthStencilView* GetDepthStencilView() { return depthStencilView.Get(); }
private:
	HWND hWnd = nullptr; // 描画対象ウィンドウ

	Microsoft::WRL::ComPtr<ID3D11Device>           device;           // デバイス
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>    immediateContext; // デバイスコンテキスト
	Microsoft::WRL::ComPtr<IDXGISwapChain>         swapChain;        // スワップチェイン
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView; // レンダーターゲットビュー
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView; // 深度ステンシル

	// 描画領域
	D3D11_VIEWPORT viewport{};

	// 画面サイズ
	float screenWidth = 0.0f;
	float screenHeight = 0.0f;

	// フルスクリーンか
	bool isFullscreen = false;
	// リサイズ中か
	bool isResizing = false;

	// ウィンドウモード時の位置とサイズ保存
	RECT windowedRect{};

	// PresentTexture用リソースの初期化
	void InitializePresentResources();

	Microsoft::WRL::ComPtr<ID3D11VertexShader>    presentVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>     presentPS;
	Microsoft::WRL::ComPtr<ID3D11SamplerState>    presentSampler;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> presentRasterizer;
};