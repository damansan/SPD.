#pragma once
#include <DirectXMath.h>


// デバッグ表示の種類
enum class PostProcessDebugView : int
{
	None = 0,         // 通常の表示
	BloomExtract = 1, // ブルーム：抜き出した明るい所
	BloomBlur = 2,    // ブルーム：ぼかした段
	BloomResult = 3,  // ブルーム：全段を足し合わせた完成品
};

// ポストエフェクトの設定値
struct PostProcessSettings
{
	// トーンマップ
	float exposure = 1.0f;       // 露出（明るさの倍率。1.0 = そのまま）
	bool  tonemapEnabled = true; // ACES トーンマップを使うか

	// カラー調整
	float contrast = 1.0f;   // コントラスト
	float saturation = 2.0f; // 彩度
	DirectX::XMFLOAT3 colorFilter = { 1.0f, 1.0f, 1.0f }; // カラーフィルタ

	// ビネット
	float vignetteIntensity = 0.2f;  // 強さ
	float vignetteSmoothness = 0.4f; // 境目のなだらかさ
	DirectX::XMFLOAT3 vignetteColor = { 0.0f, 0.0f, 0.0f }; // 隅の色

	// ブルーム
	float bloomThreshold = 1.0f; // これより明るい個所を光らせる
	float bloomIntensity = 0.5f; // 1.0 = 全段の平均を足す

	// ラジアルブラー
	bool  radialBlurEnabled = false;  // ブラーをかけるか
	float radialBlurStrength = 0.3f;  // 強度
	float radialBlurRadius = 1.2f;    // 境目
	float radialBlurSoftness = 1.3f;  // 滑らかさ

	// デバッグ表示
	PostProcessDebugView debugView = PostProcessDebugView::None;
	int debugBloomLevel = 0; // BloomBlur で表示する段
};