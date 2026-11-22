#include "ShadowMap.h"
#include <cassert>

// 初期化
void ShadowMap::Initialize(ID3D11Device* device)
{
	HRESULT hr;

	// 本体：段数ぶんの配列テクスチャ
	{
		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = RESOLUTION;
		desc.Height = RESOLUTION;
		desc.MipLevels = 1;
		desc.ArraySize = CASCADE_COUNT;
		desc.Format = DXGI_FORMAT_R32_TYPELESS;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		hr = device->CreateTexture2D(&desc, nullptr, texture.GetAddressOf());
		assert(SUCCEEDED(hr));
	}

	// 書くとき：段ごとに DSV を作る
	for (UINT i = 0; i < CASCADE_COUNT; i++)
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC desc{};
		desc.Format = DXGI_FORMAT_D32_FLOAT;
		desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.MipSlice = 0;
		desc.Texture2DArray.FirstArraySlice = i;
		desc.Texture2DArray.ArraySize = 1;
		hr = device->CreateDepthStencilView(texture.Get(), &desc, dsvs[i].GetAddressOf());
		assert(SUCCEEDED(hr));
	}

	// 読むとき：全段まとめて1つの SRV
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
		desc.Format = DXGI_FORMAT_R32_FLOAT;
		desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.MostDetailedMip = 0;
		desc.Texture2DArray.MipLevels = 1;
		desc.Texture2DArray.FirstArraySlice = 0;
		desc.Texture2DArray.ArraySize = CASCADE_COUNT;
		hr = device->CreateShaderResourceView(texture.Get(), &desc, srv.GetAddressOf());
		assert(SUCCEEDED(hr));
	}

	// 比較サンプラー：「読んだ深度 と 自分の深度」の比較を GPU にやらせる
	{
		D3D11_SAMPLER_DESC desc{};
		desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		desc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
		desc.BorderColor[0] = 1.0f;
		desc.BorderColor[1] = 1.0f;
		desc.BorderColor[2] = 1.0f;
		desc.BorderColor[3] = 1.0f;
		desc.MinLOD = 0;
		desc.MaxLOD = D3D11_FLOAT32_MAX;
		hr = device->CreateSamplerState(&desc, comparisonSampler.GetAddressOf());
		assert(SUCCEEDED(hr));
	}

	// 影を描くときのラスタライザー
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_BACK;
		desc.FrontCounterClockwise = FALSE;
		desc.DepthBias = 0;                // ステップ2で調整
		desc.SlopeScaledDepthBias = 0.0f;  // ステップ2で調整
		desc.DepthBiasClamp = 0.0f;
		desc.DepthClipEnable = FALSE;      // ライトの手前にはみ出た物も潰して描く
		hr = device->CreateRasterizerState(&desc, rasterizerState.GetAddressOf());
		assert(SUCCEEDED(hr));
	}
}

// 指定した段を描画先にする
void ShadowMap::BeginCascade(ID3D11DeviceContext* dc, UINT cascadeIndex)
{
	assert(cascadeIndex < CASCADE_COUNT);

	// 深度だけ
	dc->OMSetRenderTargets(0, nullptr, dsvs[cascadeIndex].Get());
	dc->ClearDepthStencilView(dsvs[cascadeIndex].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	D3D11_VIEWPORT vp{};
	vp.Width = static_cast<float>(RESOLUTION);
	vp.Height = static_cast<float>(RESOLUTION);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	dc->RSSetViewports(1, &vp);
}

// 描画先を外す
void ShadowMap::End(ID3D11DeviceContext* dc)
{
	dc->OMSetRenderTargets(0, nullptr, nullptr);
}
