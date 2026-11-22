#include "PostProcess.h"
#include "Engine/Resource/GpuResourceUtils.h"
#include <algorithm>
#include <cmath>

namespace
{
	// b4：GPU に送る形
	struct CbPostProcess
	{
		float exposure;
		int   tonemapEnabled; // bool は HLSL と大きさが違うので int で送る
		float contrast;
		float saturation;

		DirectX::XMFLOAT3 colorFilter; // リニア
		float vignetteIntensity;

		DirectX::XMFLOAT3 vignetteColor; // リニア
		float vignetteSmoothness;

		float aspectRatio;
		float bloomThreshold;
		int   debugView;
		float bloomScale;

		float radialBlurStrength;
		float radialBlurRadius;
		float radialBlurSoftness;
		float postPadding; // 16バイトアライメント用
	};
	static_assert(sizeof(CbPostProcess) % 16 == 0, "定数バッファは16バイトの倍数にする");

	// sRGB から リニア
	float SRGBToLinear(float c)
	{
		return (c <= 0.04045f) ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f);
	}
}

// 初期化
void PostProcess::Initialize(ID3D11Device* device)
{
	this->device = device;

	// 全画面の 頂点シェーダー
	Microsoft::WRL::ComPtr<ID3D11InputLayout> dummyLayout;
	GpuResourceUtils::LoadVertexShader(
		device, "Data/Shader/Fullscreen_VS.cso",
		nullptr, 0, dummyLayout, fullscreenVS);

	// ピクセルシェーダー
	GpuResourceUtils::LoadPixelShader(device, "Data/Shader/Final_PS.cso", finalPS);

	GpuResourceUtils::LoadPixelShader(device, "Data/Shader/BloomUpsample_PS.cso", bloomUpsamplePS);
	GpuResourceUtils::LoadPixelShader(device, "Data/Shader/BloomExtract_PS.cso", bloomExtractPS);
	GpuResourceUtils::LoadPixelShader(device, "Data/Shader/BloomDownsample_PS.cso", bloomDownsamplePS);
	GpuResourceUtils::LoadPixelShader(device, "Data/Shader/BloomBlurH_PS.cso", bloomBlurHPS);
	GpuResourceUtils::LoadPixelShader(device, "Data/Shader/BloomBlurV_PS.cso", bloomBlurVPS);
	
	GpuResourceUtils::LoadPixelShader(device, "Data/Shader/RadialBlur_PS.cso", radialBlurPS);

	// ラスタライザー
	D3D11_RASTERIZER_DESC rd{};
	rd.FillMode = D3D11_FILL_SOLID;
	rd.CullMode = D3D11_CULL_NONE;
	device->CreateRasterizerState(&rd, rasterizerState.GetAddressOf());

	// サンプラー
	D3D11_SAMPLER_DESC sd{};
	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sd.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&sd, linearSampler.GetAddressOf());

	// 設定値の定数バッファ
	GpuResourceUtils::CreateConstantBuffer(device, sizeof(CbPostProcess), cbPostProcess);
}

// HDRScene を読んで output へ書き出す
void PostProcess::Render(
	ID3D11DeviceContext* dc,
	ID3D11ShaderResourceView* hdrScene,
	ID3D11RenderTargetView* output,
	UINT width,
	UINT height,
	const PostProcessSettings& settings)
{
	// 画面の大きさが変わったら、ブルームの作業用テクスチャを作り直す
	if (width != bloomSourceWidth || height != bloomSourceHeight)
	{
		ResizeBloomTargets(width, height);
	}

	// 設定値を GPU に送る形にして渡す
	CbPostProcess cb{};
	cb.exposure = settings.exposure;
	cb.tonemapEnabled = settings.tonemapEnabled ? 1 : 0;
	cb.contrast = settings.contrast;
	cb.saturation = settings.saturation;

	// リニアに直してから送る
	cb.colorFilter =
	{
		SRGBToLinear(settings.colorFilter.x),
		SRGBToLinear(settings.colorFilter.y),
		SRGBToLinear(settings.colorFilter.z)
	};

	// ビネット
	cb.vignetteIntensity = settings.vignetteIntensity * 3.0f;
	cb.vignetteSmoothness = settings.vignetteSmoothness * 5.0f;
	cb.vignetteColor =
	{
		SRGBToLinear(settings.vignetteColor.x),
		SRGBToLinear(settings.vignetteColor.y),
		SRGBToLinear(settings.vignetteColor.z)
	};

	// 画面の縦横比
	cb.aspectRatio = (height > 0) ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;

	// ブルーム
	cb.bloomThreshold = settings.bloomThreshold;
	cb.bloomScale = settings.bloomIntensity / static_cast<float>(kBloomLevels);

	// ラジアルブラー
	cb.radialBlurStrength = settings.radialBlurStrength;
	cb.radialBlurRadius = settings.radialBlurRadius;
	cb.radialBlurSoftness = settings.radialBlurSoftness;

	// デバッグ表示
	cb.debugView = static_cast<int>(settings.debugView);

	dc->UpdateSubresource(cbPostProcess.Get(), 0, nullptr, &cb, 0, 0);
	dc->PSSetConstantBuffers(4, 1, cbPostProcess.GetAddressOf());

	// 全画面の描画で共通のステート
	dc->RSSetState(rasterizerState.Get());
	dc->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
	dc->IASetInputLayout(nullptr);
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	dc->VSSetShader(fullscreenVS.Get(), nullptr, 0);
	dc->PSSetSamplers(0, 1, linearSampler.GetAddressOf());

	// シーンを「描く先」から外す
	dc->OMSetRenderTargets(0, nullptr, nullptr);

	// この後のパスが読むシーン
	ID3D11ShaderResourceView* sceneSrv = hdrScene;

	// ラジアルブラー
	if (settings.radialBlurEnabled)
	{
		dc->PSSetShaderResources(0, 1, &hdrScene);
		DrawFullscreen(dc, radialBlurPS.Get(),
			radialBlurTarget.rtv.Get(), radialBlurTarget.width, radialBlurTarget.height);
		sceneSrv = radialBlurTarget.srv.Get();
	}

	// ブルーム：明るい所を抜き出して、1/2 の大きさへ
	dc->PSSetShaderResources(0, 1, &hdrScene);
	DrawFullscreen(dc, bloomExtractPS.Get(),
		bloomDown[0].rtv.Get(), bloomDown[0].width, bloomDown[0].height);

	// 1段ずつ小さくする
	for (int i = 1; i < kBloomLevels; i++)
	{
		ID3D11ShaderResourceView* src = bloomDown[i - 1].srv.Get();
		dc->PSSetShaderResources(0, 1, &src);
		DrawFullscreen(dc, bloomDownsamplePS.Get(),
			bloomDown[i].rtv.Get(), bloomDown[i].width, bloomDown[i].height);
	}

	// 段ごとに、横から縦の順でぼかす
	for (int i = 0; i < kBloomLevels; i++)
	{
		// 横
		ID3D11ShaderResourceView* src = bloomDown[i].srv.Get();
		dc->PSSetShaderResources(0, 1, &src);
		DrawFullscreen(dc, bloomBlurHPS.Get(),
			bloomTemp[i].rtv.Get(), bloomTemp[i].width, bloomTemp[i].height);

		// 縦
		src = bloomTemp[i].srv.Get();
		dc->PSSetShaderResources(0, 1, &src);
		DrawFullscreen(dc, bloomBlurVPS.Get(),
			bloomBlur[i].rtv.Get(), bloomBlur[i].width, bloomBlur[i].height);
	}

	// 一番小さい段から、2倍に広げながら足していく
	ID3D11ShaderResourceView* lower = bloomBlur[kBloomLevels - 1].srv.Get();
	for (int i = kBloomLevels - 2; i >= 0; i--)
	{
		ID3D11ShaderResourceView* srvs[2] = { bloomBlur[i].srv.Get(), lower };
		dc->PSSetShaderResources(0, 2, srvs);
		DrawFullscreen(dc, bloomUpsamplePS.Get(),
			bloomTemp[i].rtv.Get(), bloomTemp[i].width, bloomTemp[i].height);

		// 今書いた段が、次の段にとっての「1つ下」になる
		lower = bloomTemp[i].srv.Get();
	}

	// 最終パスで読むブルームの絵（デバッグ表示では見たい段に差し替える）
	ID3D11ShaderResourceView* bloomResult = lower; // 足し合わせた完成品（temp[0]）
	switch (settings.debugView)
	{
	case PostProcessDebugView::BloomExtract:
		bloomResult = bloomDown[0].srv.Get();
		break;
	case PostProcessDebugView::BloomBlur:
	{
		const int level = (std::clamp)(settings.debugBloomLevel, 0, kBloomLevels - 1);
		bloomResult = bloomBlur[level].srv.Get();
		break;
	}
	default: // None・BloomResult は完成品のまま
		break;
	}

	// 最終パス：シーンとブルームを読んで、表示用へ
	ID3D11ShaderResourceView* finalSrvs[2] = { sceneSrv, bloomResult };
	dc->PSSetShaderResources(0, 2, finalSrvs);
	DrawFullscreen(dc, finalPS.Get(), output, width, height);

	// 読む物の束縛を解除
	ID3D11ShaderResourceView* nullSrvs[2] = {};
	dc->PSSetShaderResources(0, 2, nullSrvs); 
}

// 画面の大きさに合わせて、ブルームの作業用テクスチャを作り直す
void PostProcess::ResizeBloomTargets(UINT width, UINT height)
{
	bloomSourceWidth = width;
	bloomSourceHeight = height;


	// ラジアルブラー
	CreateRenderTexture(radialBlurTarget, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);

	// [0] が 1/2、そこから段ごとに半分
	UINT w = width;
	UINT h = height;
	for (int i = 0; i < kBloomLevels; i++)
	{
		w = (std::max)(w / 2, 1u);
		h = (std::max)(h / 2, 1u);

		// 1を超える明るさを保てて、アルファなしで軽い形式
		CreateRenderTexture(bloomDown[i], w, h, DXGI_FORMAT_R11G11B10_FLOAT);
		CreateRenderTexture(bloomTemp[i], w, h, DXGI_FORMAT_R11G11B10_FLOAT);
		CreateRenderTexture(bloomBlur[i], w, h, DXGI_FORMAT_R11G11B10_FLOAT);
	}
}
// 作業用テクスチャを1枚作る
void PostProcess::CreateRenderTexture(RenderTexture& target, UINT width, UINT height, DXGI_FORMAT format)
{
	// 古い物を解放
	target.srv.Reset();
	target.rtv.Reset();
	target.texture.Reset();

	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	device->CreateTexture2D(&desc, nullptr, target.texture.GetAddressOf());
	device->CreateRenderTargetView(target.texture.Get(), nullptr, target.rtv.GetAddressOf());
	device->CreateShaderResourceView(target.texture.Get(), nullptr, target.srv.GetAddressOf());

	target.width = width;
	target.height = height;
}
// 全画面の四角形を1回描く
void PostProcess::DrawFullscreen(ID3D11DeviceContext* dc, ID3D11PixelShader* ps, ID3D11RenderTargetView* rtv, UINT width, UINT height)
{
	dc->OMSetRenderTargets(1, &rtv, nullptr);

	D3D11_VIEWPORT vp{};
	vp.Width = static_cast<float>(width);
	vp.Height = static_cast<float>(height);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	dc->RSSetViewports(1, &vp);

	dc->PSSetShader(ps, nullptr, 0);
	dc->Draw(4, 0);

	// 描く先を外す
	dc->OMSetRenderTargets(0, nullptr, nullptr);
}
