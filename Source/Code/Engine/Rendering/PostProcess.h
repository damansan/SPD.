#pragma once
#include <d3d11.h>
#include <wrl.h>
#include "Engine/Rendering/PostProcessSettings.h"

// ポストプロセス
class PostProcess
{
public:
	// ブルームの段数（1/2, 1/4, 1/8, 1/16, 1/32, 1/64）
	static constexpr int kBloomLevels = 6;

	// 初期化
	void Initialize(ID3D11Device* device);

	// HDRScene を読んで output へ書き出す
	void Render(
		ID3D11DeviceContext* dc,
		ID3D11ShaderResourceView* hdrScene,
		ID3D11RenderTargetView* output,
		UINT width,
		UINT height,
		const PostProcessSettings& settings);

private:
	// 作業用テクスチャ
	struct RenderTexture
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D>          texture;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   rtv;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
		UINT width = 0;
		UINT height = 0;
	};

	// 画面の大きさに合わせて、ブルームの作業用テクスチャを作り直す
	void ResizeBloomTargets(UINT width, UINT height);
	// 作業用テクスチャを1枚作る
	void CreateRenderTexture(RenderTexture& target, UINT width, UINT height, DXGI_FORMAT format);
	// 全画面の四角形を1回描く
	void DrawFullscreen(ID3D11DeviceContext* dc, ID3D11PixelShader* ps, ID3D11RenderTargetView* rtv, UINT width, UINT height);

	Microsoft::WRL::ComPtr<ID3D11Device>          device; // 作業用テクスチャを作り直すため

	Microsoft::WRL::ComPtr<ID3D11VertexShader>    fullscreenVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>     finalPS;
	
	Microsoft::WRL::ComPtr<ID3D11PixelShader>     bloomExtractPS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>     bloomDownsamplePS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>     bloomBlurHPS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>     bloomBlurVPS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>     bloomUpsamplePS;

	Microsoft::WRL::ComPtr<ID3D11PixelShader>     radialBlurPS;

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState; // カリングなし
	Microsoft::WRL::ComPtr<ID3D11SamplerState>    linearSampler;   // バイリニア・端は引き伸ばし
	Microsoft::WRL::ComPtr<ID3D11Buffer>          cbPostProcess;   // b4：設定値

	// ブルームの作業用テクスチャ
	RenderTexture bloomDown[kBloomLevels]; // 縮小した絵（[0] は抜き出した絵）
	RenderTexture bloomTemp[kBloomLevels]; // 横ぼかしの途中
	RenderTexture bloomBlur[kBloomLevels]; // 縦横ぼかした絵

	// ラジアルブラーの作業用テクスチャ
	RenderTexture radialBlurTarget;

	UINT bloomSourceWidth = 0;  // 作ったときの画面の大きさ
	UINT bloomSourceHeight = 0;
};