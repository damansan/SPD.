#pragma once
#include <vector>
#include <wrl.h>
#include <d3d11.h>
#include <DirectXMath.h>

class ShapeRenderer
{
public:
	ShapeRenderer() = default;
	ShapeRenderer(ID3D11Device* device);

	// 箱描画
	void DrawBox(
		const DirectX::XMFLOAT4X4& transform,
		const DirectX::XMFLOAT3& size,
		const DirectX::XMFLOAT4& color);

	// 球描画
	void DrawSphere(
		const DirectX::XMFLOAT3& center,
		float radius,
		const DirectX::XMFLOAT4& color);

	// カプセル描画
	void DrawCapsule(
		const DirectX::XMFLOAT4X4& transform,
		float radius,
		float halfHeight,
		const DirectX::XMFLOAT4& color);

	// 描画実行
	void Render(
		ID3D11DeviceContext* dc,
		const DirectX::XMFLOAT4X4& view,
		const DirectX::XMFLOAT4X4& projection,
		const DirectX::XMFLOAT3& lightDirection,
		bool drawWired = true);
private:
	struct Vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
	};

	struct WiredMesh
	{
		Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
		UINT vertexCount = 0;
	};

	struct SolidMesh
	{
		Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
		UINT indexCount = 0;
	};

	struct Instance
	{
		WiredMesh* wiredMesh;
		SolidMesh* solidMesh;
		DirectX::XMFLOAT4X4    worldTransform;
		DirectX::XMFLOAT4      color;
	};

	struct CbWiredMesh
	{
		DirectX::XMFLOAT4X4 viewProjection;
	};

	struct CbSolidMesh
	{
		DirectX::XMFLOAT4X4 viewProjection;
		DirectX::XMFLOAT4   lightDirection;
	};

	// GPUインスタンシング用
	struct InstanceData
	{
		DirectX::XMFLOAT4X4 world;
		DirectX::XMFLOAT4   color;
	};

	// インスタンスバッファの確保・拡張ヘルパー
	void EnsureInstanceBuffer(ID3D11Device* device, UINT requiredCount);

	// 不透明描画用のレンダーステート生成
	void CreateRenderStates(ID3D11Device* device);

	// バッファ生成ヘルパー
	void CreateWiredMesh(
		ID3D11Device* device,
		const std::vector<DirectX::XMFLOAT3>& positions,
		WiredMesh& mesh);

	void CreateSolidMesh(
		ID3D11Device* device,
		const std::vector<Vertex>& vertices,
		const std::vector<uint16_t>& indices,
		SolidMesh& mesh);

	// 箱
	void CreateWiredBoxMesh(ID3D11Device* device);
	void CreateSolidBoxMesh(ID3D11Device* device);

	// 球
	void CreateWiredSphereMesh(ID3D11Device* device, int subdivisions);
	void CreateSolidSphereMesh(ID3D11Device* device, int subdivisions);

	// 半球
	void CreateWiredHalfSphereMesh(ID3D11Device* device, int subdivisions);
	void CreateSolidHalfSphereMesh(ID3D11Device* device, int subdivisions);

	// 円柱
	void CreateWiredCylinderMesh(ID3D11Device* device, int subdivisions);
	void CreateSolidCylinderMesh(ID3D11Device* device, int subdivisions);

private:
	// 単位サイズのメッシュ
	WiredMesh wiredBoxMesh;
	WiredMesh wiredSphereMesh;
	WiredMesh wiredHalfSphereMesh;
	WiredMesh wiredCylinderMesh;

	SolidMesh solidBoxMesh;
	SolidMesh solidSphereMesh;
	SolidMesh solidHalfSphereMesh;
	SolidMesh solidCylinderMesh;

	std::vector<Instance> instances;

	// GPUインスタンシング用バッファ
	Microsoft::WRL::ComPtr<ID3D11Buffer> instanceBuffer;
	UINT instanceBufferCapacity = 0;

	// 不透明描画用ステート(Wired用)
	Microsoft::WRL::ComPtr<ID3D11BlendState>        opaqueBlendState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> opaqueDepthStencilState;

	// 半透明描画用ステート(Solid用)
	Microsoft::WRL::ComPtr<ID3D11BlendState>        alphaBlendState;

	// Wiredシェーダー
	Microsoft::WRL::ComPtr<ID3D11VertexShader> wiredVertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  wiredPixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>  wiredInputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer>       wiredConstantBuffer;

	// Solidシェーダー
	Microsoft::WRL::ComPtr<ID3D11VertexShader> solidVertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  solidPixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>  solidInputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer>       solidConstantBuffer;
};