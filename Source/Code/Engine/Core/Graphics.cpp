#include "Graphics.h"
#include "Engine/Utility/misc.h"
#include "Engine/Resource/GpuResourceUtils.h"

// 初期化
void Graphics::Initialize(HWND hWnd)
{
	// ウィンドウハンドル保存
	this->hWnd = hWnd;

	// クライアント領域サイズ取得
	RECT rc;
	// ウィンドウ内部サイズ取得
	GetClientRect(hWnd, &rc);
	UINT w = rc.right - rc.left; // 幅
	UINT h = rc.bottom - rc.top; // 高さ
	// 保存
	screenWidth = static_cast<float>(w);
	screenHeight = static_cast<float>(h);

	// HRESULT保存
	HRESULT hr{ S_OK };

	// デバイス生成フラグ
	UINT createDeviceFlags = 0;


#ifdef _DEBUG 
	// デバッグレイヤー有効化
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// DirectX11.0機能レベルで動作
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
	};

	// スワップチェイン設定
	DXGI_SWAP_CHAIN_DESC scDesc{};
	scDesc.BufferCount = 2;                                // バッファ数2
	scDesc.BufferDesc.Width = w;                           // バッファ幅
	scDesc.BufferDesc.Height = h;                          // バッファ高さ
	scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA8bitフォーマット
	scDesc.BufferDesc.RefreshRate.Numerator = 60;          // リフレッシュレート60Hz
	scDesc.BufferDesc.RefreshRate.Denominator = 1;         // 分母（60/1=60Hz）
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;  // レンダーターゲット用途
	scDesc.OutputWindow = hWnd;                            // 描画対象ウィンドウ
	scDesc.SampleDesc.Count = 1;                           // MSAAサンプル数
	scDesc.SampleDesc.Quality = 0;                         // 品質レベル	
	scDesc.Windowed = TRUE;                                // ウィンドウモード
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;     // FlipDiscard方式
	scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;     // テアリング許可


#if 0
	// デバイス/スワップチェイン生成
	D3D_FEATURE_LEVEL featureLevel;
	hr = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		createDeviceFlags,
		featureLevels,
		ARRAYSIZE(featureLevels),
		D3D11_SDK_VERSION,
		&scDesc,
		swapChain.GetAddressOf(),
		device.GetAddressOf(),
		&featureLevel,
		immediateContext.GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

#else
	// アダプター選択
	Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
	CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)factory.GetAddressOf());

	Microsoft::WRL::ComPtr<IDXGIAdapter1> selectedAdapter;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

	for (UINT i = 0; factory->EnumAdapters1(i, adapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND; i++)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		// ソフトウェアアダプターは除外
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			adapter.Reset();
			continue;
		}

		// NVIDIA or AMD (即決定)
		if (desc.VendorId == 0x10DE || desc.VendorId == 0x1002)
		{
			selectedAdapter = adapter;
			break;
		}
		adapter.Reset();
	}

	// デバイス/スワップチェイン生成
	D3D_FEATURE_LEVEL featureLevel;
	hr = D3D11CreateDeviceAndSwapChain(
		selectedAdapter.Get(),
		selectedAdapter ? D3D_DRIVER_TYPE_UNKNOWN    // NVIDIA/AMD見つかった
		: D3D_DRIVER_TYPE_HARDWARE,                  // 見つからなかったらWindowsに任せる
		nullptr,
		createDeviceFlags,
		featureLevels,
		ARRAYSIZE(featureLevels),
		D3D11_SDK_VERSION,
		&scDesc,
		swapChain.GetAddressOf(),
		device.GetAddressOf(),
		&featureLevel,
		immediateContext.GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

#endif


	// レンダーターゲットビュー生成
	// スワップチェインのバックバッファ取得
	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
	hr = swapChain->GetBuffer(
		0,
		__uuidof(ID3D11Texture2D),
		reinterpret_cast<void**>(backBuffer.GetAddressOf())
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// バックバッファからレンダーターゲットビュー生成
	hr = device->CreateRenderTargetView(
		backBuffer.Get(),
		nullptr,
		renderTargetView.GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));


	// 深度ステンシル生成
	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilBuffer; // 深度バッファテクスチャ
	D3D11_TEXTURE2D_DESC dsDesc{};
	dsDesc.Width = w;                   // テクスチャサイズ(幅)
	dsDesc.Height = h;                  // テクスチャサイズ(高さ)
	dsDesc.MipLevels = 1;               // ミップマップ数
	dsDesc.ArraySize = 1;               // 配列サイズ
	dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // Depth24 + Stencil8
	dsDesc.SampleDesc.Count = 1;        // MSAA設定
	dsDesc.SampleDesc.Quality = 0;      // MSAAを使わない場合は0固定
	dsDesc.Usage = D3D11_USAGE_DEFAULT; // GPU読み書き用
	dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	// 深度ステンシルテクスチャ生成
	hr = device->CreateTexture2D(
		&dsDesc,
		nullptr,
		depthStencilBuffer.GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// デプスステンシルビュー生成
	hr = device->CreateDepthStencilView(
		depthStencilBuffer.Get(),
		nullptr,
		depthStencilView.GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// ビューポート設定
	viewport.TopLeftX = 0.0f;       // 左上座標
	viewport.TopLeftY = 0.0f;
	viewport.Width = screenWidth;   // 描画領域サイズ
	viewport.Height = screenHeight;
	viewport.MinDepth = 0.0f;       // 深度範囲
	viewport.MaxDepth = 1.0f;


	// PresentTexture用リソース初期化
	InitializePresentResources();
}

// 画面クリア
void Graphics::Clear(float r, float g, float b, float a)
{
	// クリアカラー作成
	float color[4]{ r, g, b, a };

	// バックバッファを指定色でクリア
	immediateContext->ClearRenderTargetView(renderTargetView.Get(), color);
	// 深度ステンシル値初期化
	immediateContext->ClearDepthStencilView(
		depthStencilView.Get(),
		D3D11_CLEAR_DEPTH |
		D3D11_CLEAR_STENCIL,
		1.0f, // 深度初期値(1.0 = 最遠)
		0     // ステンシル初期値
	);
}

// レンダーターゲット設定
void Graphics::SetRenderTargets()
{
	// ビューポート設定
	immediateContext->RSSetViewports(1, &viewport);
	// レンダーターゲットと深度ステンシルをOMステージに設定
	immediateContext->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());
}

// 指定テクスチャをフルスクリーンでバックバッファへ描画
void Graphics::PresentTexture(ID3D11ShaderResourceView* srv)
{
	if (!srv) return;

	// バックバッファへ描画
	immediateContext->RSSetViewports(1, &viewport);
	immediateContext->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), nullptr);

	// ブレンド・深度ステートを既定に戻す
	immediateContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	immediateContext->OMSetDepthStencilState(nullptr, 1);

	// シェーダー設定
	immediateContext->VSSetShader(presentVS.Get(), nullptr, 0);
	immediateContext->PSSetShader(presentPS.Get(), nullptr, 0);

	// 入力レイアウトなし・フルスクリーン三角形ストリップ
	immediateContext->IASetInputLayout(nullptr);
	immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	immediateContext->RSSetState(presentRasterizer.Get());

	// テクスチャ・サンプラー設定
	immediateContext->PSSetShaderResources(0, 1, &srv);
	immediateContext->PSSetSamplers(0, 1, presentSampler.GetAddressOf());

	// フルスクリーン描画
	immediateContext->Draw(4, 0);

	// SRVの束縛解除
	ID3D11ShaderResourceView* nullSrv = nullptr;
	immediateContext->PSSetShaderResources(0, 1, &nullSrv);
}

// 画面プレゼント
void Graphics::Present(bool vsync)
{
	//swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING); // 常にティアリング許可

	if (vsync)
	{
		// VSync ON：モニターのリフレッシュレートに同期
		swapChain->Present(1, 0);
	}
	else
	{
		// VSync OFF：可能な限り速く描画
		swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	}
}

// フルスクリーン切り替え
void Graphics::ToggleFullscreen()
{
	// リサイズ中
	isResizing = true;

	// フルスクリーン切り替え
	if (!isFullscreen)
	{
		// 現在ウィンドウ位置保存
		GetWindowRect(hWnd, &windowedRect);
		// 現在モニター取得
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		// モニター情報取得
		MONITORINFO monitorInfo{ sizeof(MONITORINFO) };
		GetMonitorInfo(monitor, &monitorInfo);
		// モニター矩形
		RECT& mr = monitorInfo.rcMonitor;

		// 完全フルスクリーン用
		SetWindowLong(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE); // タイトルバーや枠なしの完全フルスクリーン用スタイル

		// モニター全体サイズへ変更
		SetWindowPos(
			hWnd,
			HWND_TOP,
			mr.left, mr.top,
			mr.right - mr.left, mr.bottom - mr.top,
			SWP_FRAMECHANGED
		);

		// フルスクリーン中
		isFullscreen = true;
	}
	// ウィンドウモードへ戻す
	else
	{
		// ウィンドウスタイルに戻す
		SetWindowLong(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
		// 保存していた位置へ戻す
		SetWindowPos(hWnd, HWND_TOP,
			windowedRect.left, windowedRect.top,
			windowedRect.right - windowedRect.left,
			windowedRect.bottom - windowedRect.top,
			SWP_FRAMECHANGED
		);

		// ウィンドウ中
		isFullscreen = false;
	}

	// ウィンドウ更新
	UpdateWindow(hWnd);

	// バッファ再生成
	ResizeBuffers();

	// リサイズ終了
	isResizing = false;
}

// バッファ再生成
void Graphics::ResizeBuffers()
{
	// 現在クライアントサイズ取得
	RECT rc;
	GetClientRect(hWnd, &rc);
	UINT w = rc.right - rc.left;
	UINT h = rc.bottom - rc.top;
	// 最小化時スキップ
	if (w == 0 || h == 0) return;

	// レンダーターゲット解除
	immediateContext->OMSetRenderTargets(0, nullptr, nullptr);

	// バックバッファレンダーターゲットビュー解放
	renderTargetView.Reset();
	// 深度ステンシル解放
	depthStencilView.Reset();

	// スワップチェイン再生成
	swapChain->ResizeBuffers(
		0,
		w, h,
		DXGI_FORMAT_UNKNOWN,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING // 常にティアリング許可
	);

	// レンダーターゲットビュー再生成
	// 新バックバッファ取得
	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
	swapChain->GetBuffer(
		0,
		__uuidof(ID3D11Texture2D),
		reinterpret_cast<void**>(backBuffer.GetAddressOf())
	);
	// バックバッファ用レンダーターゲットビュー生成
	device->CreateRenderTargetView(
		backBuffer.Get(),
		nullptr,
		renderTargetView.GetAddressOf()
	);

	// 深度ステンシルビュー再生成
	// 新深度バッファ生成
	Microsoft::WRL::ComPtr<ID3D11Texture2D> dsBuffer;
	D3D11_TEXTURE2D_DESC dsDesc{};
	dsDesc.Width = w;                   // バッファサイズ
	dsDesc.Height = h;                  // バッファサイズ
	dsDesc.MipLevels = 1;               // ミップ無し
	dsDesc.ArraySize = 1;               // 配列サイズ
	dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // Depth24 + Stencil8
	dsDesc.SampleDesc.Count = 1;        // MSAA設定
	dsDesc.Usage = D3D11_USAGE_DEFAULT; // GPU使用
	dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL; // 深度ステンシル用途

	// 深度バッファ生成
	device->CreateTexture2D(
		&dsDesc,
		nullptr,
		dsBuffer.GetAddressOf()
	);

	// デプスステンシルビュー生成
	device->CreateDepthStencilView(
		dsBuffer.Get(),
		nullptr,
		depthStencilView.GetAddressOf()
	);

	// 画面サイズ更新
	screenWidth = static_cast<float>(w);
	screenHeight = static_cast<float>(h);
	// ビューポートサイズ更新
	viewport.Width = screenWidth;
	viewport.Height = screenHeight;
}

// 終了処理
void Graphics::Cleanup()
{
	if (immediateContext)
	{
		immediateContext->OMSetRenderTargets(0, nullptr, nullptr);
		immediateContext->ClearState();
	}

	renderTargetView.Reset();
	depthStencilView.Reset();
}

// PresentTexture用リソースの初期化
void Graphics::InitializePresentResources()
{
	// 頂点シェーダー
	Microsoft::WRL::ComPtr<ID3D11InputLayout> dummyLayout;
	GpuResourceUtils::LoadVertexShader(
		device.Get(), "Data/Shader/Fullscreen_VS.cso",
		nullptr, 0, dummyLayout, presentVS
	);

	// ピクセルシェーダー
	GpuResourceUtils::LoadPixelShader(
		device.Get(), "Data/Shader/Present_PS.cso", presentPS
	);

	// サンプラーステート
	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&samplerDesc, presentSampler.GetAddressOf());

	// ラスタライザーステート
	D3D11_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	device->CreateRasterizerState(&rasterizerDesc, presentRasterizer.GetAddressOf());
}