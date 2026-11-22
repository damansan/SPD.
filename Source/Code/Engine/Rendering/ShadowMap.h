#pragma once
#include <d3d11.h>
#include <wrl.h>
#include "Engine/Rendering/ShaderLimits.h"

// カスケードシャドウ用の影マップ
class ShadowMap
{
public:
	static constexpr UINT CASCADE_COUNT = SHADOW_CASCADE_COUNT; // カスケードの段数
	static constexpr UINT RESOLUTION = 2048;  // 1段あたりの縦横ピクセル数

	// 初期化
	void Initialize(ID3D11Device* device);

	// 指定した段を描画先にする
	void BeginCascade(ID3D11DeviceContext* dc, UINT cascadeIndex);

	// 描画先を外す
	void End(ID3D11DeviceContext* dc);

	ID3D11ShaderResourceView* GetSRV() const { return srv.Get(); }
	ID3D11SamplerState* GetComparisonSampler() const { return comparisonSampler.Get(); }
	ID3D11RasterizerState* GetRasterizerState() const { return rasterizerState.Get(); }

private:
	Microsoft::WRL::ComPtr<ID3D11Texture2D>          texture;             // 本体（配列テクスチャ）
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   dsvs[CASCADE_COUNT]; // 書くとき：段ごとに1枚
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;                 // 読むとき：全段まとめて1つ
	Microsoft::WRL::ComPtr<ID3D11SamplerState>       comparisonSampler;   // 影判定用
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>    rasterizerState;     // 影を描くとき用
};