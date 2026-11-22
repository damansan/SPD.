#pragma once
#include <d3d11.h>
#include <wrl.h>

#include "entt.hpp"

#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Components/StaticMeshComponent.h"
#include "Engine/ECS/Components/SkinnedMeshComponent.h"
#include "Engine/ECS/Components/MaterialComponent.h"
#include "Engine/ECS/Components/Tags.h"

#include "Engine/Resource/Model.h"
#include "Engine/Rendering/GBuffer.h"
#include "Engine/Rendering/ShadowMap.h"
#include "Engine/Rendering/ShaderLimits.h"

// 定数バッファ構造体
struct CbScene
{
	DirectX::XMFLOAT4X4 view;             // ビュー行列
	DirectX::XMFLOAT4X4 projection;       // プロジェクション行列
	DirectX::XMFLOAT4   cameraPosition;   // カメラ位置（w未使用）
};

struct CbObject
{
	DirectX::XMFLOAT4X4 world;                 // ワールド行列
	DirectX::XMFLOAT4X4 worldInverseTranspose; // 法線用

	float               alphaCutoff;           // Mask素材のクリップしきい値
	float               metallicFactor;        // 金属度の係数
	float               roughnessFactor;       // 粗さの係数
	float               padding0;              // 16バイトアライメント用

	DirectX::XMFLOAT4   baseColor;             // マテリアルの基本色

	DirectX::XMFLOAT3   emissiveFactor;        // 発光の係数

	float               padding1;              // 16バイトアライメント用
};

struct CbSkinning
{
	DirectX::XMFLOAT4X4 boneTransforms[MAX_BONES];
};

// b5：影を読む側
struct CbShadow
{
	DirectX::XMFLOAT4X4 shadowViewProjections[SHADOW_CASCADE_COUNT]; // 各段のライトVP

	DirectX::XMFLOAT4   cascadeSplits;  // xyz = 各段の奥の端（カメラからの距離 m）

	float               shadowStrength; // 影の中で環境光をどれだけ暗くするか
	DirectX::XMFLOAT3   shadowPadding;  // 16バイトアライメント用
};

// b6：影を描く側
struct CbShadowCaster
{
	DirectX::XMFLOAT4X4 lightViewProjection; // いま描いている段のライトVP
};

class RenderSystem
{
public:
	// 初期化
	void Initialize(ID3D11Device* device);

	// 描画
	void Render(
		entt::registry& registry,
		ID3D11DeviceContext* dc,
		ID3D11RenderTargetView* rtv,
		ID3D11DepthStencilView* dsv,
		ID3D11Device* device,
		UINT width,
		UINT height
	);

	// フラスタムをScene側からセット
	void SetFrustum(
		const DirectX::XMFLOAT4X4& view,
		const DirectX::XMFLOAT4X4& projection)
	{
		cachedFrustum = CalcFrustum(view, projection);
	}

	// 影の設定をScene側からセット
	void SetShadowParams(
		const DirectX::XMFLOAT3& lightDirection,
		const DirectX::XMFLOAT4X4& cameraView,
		const DirectX::XMFLOAT4X4& cameraProjection)
	{
		shadowLightDirection = lightDirection;
		shadowCameraView = cameraView;
		shadowCameraProjection = cameraProjection;
	}

	// 影の濃さ
	void SetShadowStrength(float strength) { shadowStrength = strength; }

	// IBLテクスチャセット
	void SetIBLTextures(
		ID3D11ShaderResourceView* diffuseIem,
		ID3D11ShaderResourceView* specularPmrem,
		ID3D11ShaderResourceView* lutGgx)
	{
		iblDiffuseIem = diffuseIem;
		iblSpecularPmrem = specularPmrem;
		iblLutGgx = lutGgx;
	}

	// 定数バッファゲッター
	ID3D11Buffer* GetCbScene() { return cbScene.Get(); }
	ID3D11Buffer** GetCbSceneAddress() { return cbScene.GetAddressOf(); }

private:

	// フラスタム平面
	struct FrustumPlane
	{
		DirectX::XMFLOAT3 normal;
		float distance;
	};

	// フラスタム（6平面）
	struct Frustum
	{
		FrustumPlane planes[6]; // Left, Right, Top, Bottom, Near, Far
	};

	// フラスタム計算（ViewProjection行列から）
	Frustum CalcFrustum(
		const DirectX::XMFLOAT4X4& view,
		const DirectX::XMFLOAT4X4& projection
	);
	// フラスタム計算
	Frustum CalcFrustum(const DirectX::XMFLOAT4X4& viewProjection);

	// バウンディングボックスがフラスタム内にあるか判定
	bool IsInFrustum(
		const Frustum& frustum,
		const Model::BoundingBox& bbox,
		const DirectX::XMFLOAT4X4& world
	);

	// カスケードの分割距離と、各段のライト行列を計算する
	void CalcCascades();

	// Shadowパス
	void ShadowPass(
		entt::registry& registry,
		ID3D11DeviceContext* dc
	);

	// Geometryパス
	void GeometryPass(
		entt::registry& registry,
		ID3D11DeviceContext* dc,
		ID3D11DepthStencilView* dsv,
		const Frustum& frustum
	);

	// Lightingパス
	void LightingPass(
		ID3D11DeviceContext* dc,
		ID3D11RenderTargetView* rtv,
		ID3D11DepthStencilView* dsv
	);

	// 半透明パス(フォワードレンダリング)
	void ForwardTransparentPass(
		entt::registry& registry,
		ID3D11DeviceContext* dc,
		ID3D11RenderTargetView* rtv,
		ID3D11DepthStencilView* dsv,
		const Frustum& frustum
	);


	Microsoft::WRL::ComPtr<ID3D11VertexShader> staticVertexShader;   // 静的メッシュ用
	Microsoft::WRL::ComPtr<ID3D11VertexShader> skinnedVertexShader;  // スキニングメッシュ用
	Microsoft::WRL::ComPtr<ID3D11VertexShader> lightingVS;           // Lightingパス用頂点シェーダー
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  lightingPS;           // Lightingパス用ピクセルシェーダー
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  forwardTransparentPS; // 半透明フォワード用(PBR)

	Microsoft::WRL::ComPtr<ID3D11VertexShader> shadowStaticVS;       // 影を描く（静的メッシュ）
	Microsoft::WRL::ComPtr<ID3D11VertexShader> shadowSkinnedVS;      // 影を描く（スキニングメッシュ）

	// GBufferシェーダーマップ
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11PixelShader>> gBufferPixelShaders;
	
	// 入力レイアウト
	Microsoft::WRL::ComPtr<ID3D11InputLayout>  inputLayout;



	// サンプラーステート
	Microsoft::WRL::ComPtr<ID3D11SamplerState>      samplerState;
	// ラスタライザーステート
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>   rasterizerState;
	// ラスタライザーステート(フルスクリーン描画用)
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>   rasterizerStateNoCull;

	// 深度ステンシルステート
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;  
	// 深度ステンシルステート(半透明用)
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilStateTransparent; 
	
	// GeometryPass
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilStateGeometry;   
	// LightingPass
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilStateLighting;

	// アルファブレンド(半透明用)
	Microsoft::WRL::ComPtr<ID3D11BlendState> blendState;
	// GBuffer用(ブレンドなし)
	Microsoft::WRL::ComPtr<ID3D11BlendState> blendStateOpaque;


	// 定数バッファ
	Microsoft::WRL::ComPtr<ID3D11Buffer> cbScene;    // シーン情報
	Microsoft::WRL::ComPtr<ID3D11Buffer> cbObject;   // オブジェクト情報
	Microsoft::WRL::ComPtr<ID3D11Buffer> cbSkinning; // スキニング定数バッファ
	Microsoft::WRL::ComPtr<ID3D11Buffer> cbShadow;       // 影を読む側（b5）
	Microsoft::WRL::ComPtr<ID3D11Buffer> cbShadowCaster; // 影を描く側（b6）

	// ダミーテクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> dummyWhite;  // Albedo/Occlusion用
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> dummyNormal; // 法線マップ用
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> dummyBlack;  // Emissive用

	// IBL用テクスチャ
	ID3D11ShaderResourceView* iblDiffuseIem    = nullptr;
	ID3D11ShaderResourceView* iblSpecularPmrem = nullptr;
	ID3D11ShaderResourceView* iblLutGgx        = nullptr;


	// フラスタム
	Frustum cachedFrustum;
	// GBuffer
	GBuffer gBuffer;
	// 影マップ
	ShadowMap shadowMap;
	
	// 影の設定
	DirectX::XMFLOAT3   shadowLightDirection = { 0.5f, -1.0f, 0.5f }; // 光の進む向き
	DirectX::XMFLOAT4X4 shadowCameraView{};                           // 影を合わせるカメラ
	DirectX::XMFLOAT4X4 shadowCameraProjection{};

	// カスケードの調整値
	static constexpr float SHADOW_DISTANCE = 60.0f;      // 影を出す最大距離
	static constexpr float SHADOW_SPLIT_LAMBDA = 0.75f;  // 分割の偏り
	static constexpr float SHADOW_CASTER_MARGIN = 50.0f; // ライト側へ伸ばす距離

	// CalcCascades の結果
	DirectX::XMFLOAT4X4 cascadeViewProjections[SHADOW_CASCADE_COUNT]{};
	float               cascadeSplits[SHADOW_CASCADE_COUNT]{};

	// 影の濃さ
	float shadowStrength = 0.6f;

	// 画面サイズ
	UINT screenWidth = 0;
	UINT screenHeight = 0;
};