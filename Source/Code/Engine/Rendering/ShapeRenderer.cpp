#include "ShapeRenderer.h"
#include "Engine/Resource/GpuResourceUtils.h"
#include <algorithm>

static const float PI = DirectX::XM_PI;
static const int   SUBDIVISIONS = 16;

ShapeRenderer::ShapeRenderer(ID3D11Device* device)
{
	// Wiredシェーダー
	{
		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA,   0 },
			{ "WORLD",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "WORLD",    1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "WORLD",    2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "WORLD",    3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "COLOR",    1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		};

		GpuResourceUtils::LoadVertexShader(device, "Data/Shader/WiredShape_VS.cso", layout, _countof(layout), wiredInputLayout, wiredVertexShader);
		GpuResourceUtils::LoadPixelShader(device, "Data/Shader/WiredShape_PS.cso", wiredPixelShader);
		GpuResourceUtils::CreateConstantBuffer(device, sizeof(CbWiredMesh), wiredConstantBuffer);
	}

	// Solidシェーダー
	{
		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,                            D3D11_INPUT_PER_VERTEX_DATA,   0 },
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
			{ "WORLD",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,                            D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "WORLD",    1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16,                           D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "WORLD",    2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32,                           D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "WORLD",    3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48,                           D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "COLOR",    1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64,                           D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		};
		GpuResourceUtils::LoadVertexShader(device, "Data/Shader/SolidShape_VS.cso", layout, _countof(layout), solidInputLayout, solidVertexShader);
		GpuResourceUtils::LoadPixelShader(device, "Data/Shader/SolidShape_PS.cso", solidPixelShader);
		GpuResourceUtils::CreateConstantBuffer(device, sizeof(CbSolidMesh), solidConstantBuffer);
	}

	// 単位メッシュを事前生成
	CreateWiredBoxMesh(device);
	CreateSolidBoxMesh(device);
	CreateWiredSphereMesh(device, SUBDIVISIONS);
	CreateSolidSphereMesh(device, SUBDIVISIONS);
	CreateWiredHalfSphereMesh(device, SUBDIVISIONS);
	CreateSolidHalfSphereMesh(device, SUBDIVISIONS);
	CreateWiredCylinderMesh(device, SUBDIVISIONS);
	CreateSolidCylinderMesh(device, SUBDIVISIONS);

	// 不透明描画用ステート生成
	CreateRenderStates(device);
}

// 箱描画
void ShapeRenderer::DrawBox(
	const DirectX::XMFLOAT4X4& transform,
	const DirectX::XMFLOAT3& size,
	const DirectX::XMFLOAT4& color)
{
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(size.x, size.y, size.z);
	DirectX::XMMATRIX W = DirectX::XMLoadFloat4x4(&transform);

	Instance inst;
	inst.wiredMesh = &wiredBoxMesh;
	inst.solidMesh = &solidBoxMesh;
	DirectX::XMStoreFloat4x4(&inst.worldTransform, S * W);
	inst.color = color;
	instances.push_back(inst);
}
// 球描画
void ShapeRenderer::DrawSphere(
	const DirectX::XMFLOAT3& center,
	float radius,
	const DirectX::XMFLOAT4& color)
{
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(radius, radius, radius);
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(center.x, center.y, center.z);

	Instance inst;
	inst.wiredMesh = &wiredSphereMesh;
	inst.solidMesh = &solidSphereMesh;
	DirectX::XMStoreFloat4x4(&inst.worldTransform, S * T);
	inst.color = color;
	instances.push_back(inst);
}
// カプセル描画
void ShapeRenderer::DrawCapsule(
	const DirectX::XMFLOAT4X4& transform,
	float radius,
	float halfHeight,
	const DirectX::XMFLOAT4& color)
{
	DirectX::XMMATRIX base = DirectX::XMLoadFloat4x4(&transform);
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(radius, radius, radius);
	DirectX::XMMATRIX flip = DirectX::XMMatrixRotationX(DirectX::XM_PI);

	// 上半球
	{
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(0, halfHeight, 0);
		Instance inst;
		inst.wiredMesh = &wiredHalfSphereMesh;
		inst.solidMesh = &solidHalfSphereMesh;
		DirectX::XMStoreFloat4x4(&inst.worldTransform, S * T * base);
		inst.color = color;
		instances.push_back(inst);
	}
	// 下半球
	{
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(0, -halfHeight, 0);
		Instance inst;
		inst.wiredMesh = &wiredHalfSphereMesh;
		inst.solidMesh = &solidHalfSphereMesh;
		DirectX::XMStoreFloat4x4(&inst.worldTransform, S * flip * T * base);
		inst.color = color;
		instances.push_back(inst);
	}
	// 円柱
	{
		DirectX::XMMATRIX cylS = DirectX::XMMatrixScaling(radius, halfHeight * 2.0f, radius);
		Instance inst;
		inst.wiredMesh = &wiredCylinderMesh;
		inst.solidMesh = &solidCylinderMesh;
		DirectX::XMStoreFloat4x4(&inst.worldTransform, cylS * base);
		inst.color = color;
		instances.push_back(inst);
	}
}

// 描画
void ShapeRenderer::Render(
	ID3D11DeviceContext* dc,
	const DirectX::XMFLOAT4X4& view,
	const DirectX::XMFLOAT4X4& projection,
	const DirectX::XMFLOAT3& lightDirection,
	bool drawWired) 
{
	if (instances.empty()) return;

	DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&view);
	DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&projection);
	DirectX::XMMATRIX VP = V * P;

	// メッシュの種類ごとにグループ化
	struct InstanceGroup
	{
		WiredMesh* wiredMesh;
		SolidMesh* solidMesh;
		UINT instanceOffset;
		UINT instanceCount;
	};

	std::sort(instances.begin(), instances.end(),
		[](const Instance& a, const Instance& b)
		{
			return a.wiredMesh < b.wiredMesh;
		});

	std::vector<InstanceGroup> groups;
	std::vector<InstanceData> instanceData;
	instanceData.reserve(instances.size());

	for (auto& inst : instances)
	{
		// 既存グループを探す
		InstanceGroup* group = nullptr;
		for (auto& g : groups)
		{
			if (g.wiredMesh == inst.wiredMesh)
			{
				group = &g;
				break;
			}
		}
		if (!group)
		{
			groups.push_back({ inst.wiredMesh, inst.solidMesh, static_cast<UINT>(instanceData.size()), 0 });
			group = &groups.back();
		}

		InstanceData data;
		data.world = inst.worldTransform;
		data.color = inst.color;
		instanceData.push_back(data);
		group->instanceCount++;
	}

	// インスタンスバッファへ一括転送
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	dc->GetDevice(device.GetAddressOf());
	EnsureInstanceBuffer(device.Get(), static_cast<UINT>(instanceData.size()));

	D3D11_MAPPED_SUBRESOURCE mapped;
	dc->Map(instanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	memcpy(mapped.pData, instanceData.data(), sizeof(InstanceData)* instanceData.size());
	dc->Unmap(instanceBuffer.Get(), 0);

	// 描画前に不透明用のステートへ明示的に切り替える
	float blendFactor[4] = { 0, 0, 0, 0 };
	dc->OMSetBlendState(opaqueBlendState.Get(), blendFactor, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(opaqueDepthStencilState.Get(), 0);

	if (drawWired)
	// Wiredパス
	{
		CbWiredMesh cb;
		DirectX::XMStoreFloat4x4(&cb.viewProjection, VP);
		dc->UpdateSubresource(wiredConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

		dc->VSSetShader(wiredVertexShader.Get(), nullptr, 0);
		dc->PSSetShader(wiredPixelShader.Get(), nullptr, 0);
		dc->IASetInputLayout(wiredInputLayout.Get());
		dc->VSSetConstantBuffers(0, 1, wiredConstantBuffer.GetAddressOf());
		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

		for (auto& g : groups)
		{
			ID3D11Buffer* buffers[2] = { g.wiredMesh->vertexBuffer.Get(), instanceBuffer.Get() };
			UINT strides[2] = { sizeof(DirectX::XMFLOAT3), sizeof(InstanceData) };
			UINT offsets[2] = { 0, g.instanceOffset * sizeof(InstanceData) };
			dc->IASetVertexBuffers(0, 2, buffers, strides, offsets);
			dc->DrawInstanced(g.wiredMesh->vertexCount, g.instanceCount, 0, 0);
		}
	}

	// Solidパス
	{
		CbSolidMesh cb;
		DirectX::XMStoreFloat4x4(&cb.viewProjection, VP);
		cb.lightDirection = { lightDirection.x, lightDirection.y, lightDirection.z, 0 };
		dc->UpdateSubresource(solidConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
		dc->VSSetConstantBuffers(0, 1, solidConstantBuffer.GetAddressOf());
		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		dc->VSSetShader(solidVertexShader.Get(), nullptr, 0);
		dc->PSSetShader(solidPixelShader.Get(), nullptr, 0);
		dc->IASetInputLayout(solidInputLayout.Get());
		dc->OMSetBlendState(alphaBlendState.Get(), blendFactor, 0xFFFFFFFF);
		dc->OMSetDepthStencilState(opaqueDepthStencilState.Get(), 0);

		for (auto& g : groups)
		{
			ID3D11Buffer* buffers[2] = { g.solidMesh->vertexBuffer.Get(), instanceBuffer.Get() };
			UINT strides[2] = { sizeof(Vertex), sizeof(InstanceData) };
			UINT offsets[2] = { 0, g.instanceOffset * sizeof(InstanceData) };
			dc->IASetVertexBuffers(0, 2, buffers, strides, offsets);
			dc->IASetIndexBuffer(g.solidMesh->indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
			dc->DrawIndexedInstanced(g.solidMesh->indexCount, g.instanceCount, 0, 0, 0);
		}
	}

	instances.clear();
}

// インスタンスバッファの確保・拡張ヘルパー
void ShapeRenderer::EnsureInstanceBuffer(ID3D11Device* device, UINT requiredCount)
{
	// 既に十分な容量があれば何もしない
	if (requiredCount <= instanceBufferCapacity) return;

	// 少し余裕を持たせて確保する(毎フレーム微妙に増減するケースで頻繁に作り直しが起きないようにするため)
	UINT newCapacity = requiredCount + requiredCount / 2;

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = sizeof(InstanceData) * newCapacity;
	desc.Usage = D3D11_USAGE_DYNAMIC;          // CPUから毎フレーム書き込むため
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER; // インスタンスバッファも「頂点バッファ」として扱う
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	instanceBuffer.Reset();
	HRESULT hr = device->CreateBuffer(&desc, nullptr, instanceBuffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), L"ShapeRenderer: InstanceBuffer failed");

	instanceBufferCapacity = newCapacity;
}

// 不透明描画用のレンダーステート生成
void ShapeRenderer::CreateRenderStates(ID3D11Device* device)
{
	// ブレンドなし(不透明、Wired用)
	{
		D3D11_BLEND_DESC desc = {};
		desc.RenderTarget[0].BlendEnable = FALSE;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
		HRESULT hr = device->CreateBlendState(&desc, opaqueBlendState.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), L"ShapeRenderer: BlendState failed");
	}

	// アルファブレンドあり(半透明、Solid用)
	{
		D3D11_BLEND_DESC desc = {};
		desc.RenderTarget[0].BlendEnable = TRUE;
		desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
		HRESULT hr = device->CreateBlendState(&desc, alphaBlendState.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), L"ShapeRenderer: AlphaBlendState failed");
	}

	// 深度テスト・書き込みあり
	{
		D3D11_DEPTH_STENCIL_DESC desc = {};
		desc.DepthEnable = TRUE;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		HRESULT hr = device->CreateDepthStencilState(&desc, opaqueDepthStencilState.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), L"ShapeRenderer: DepthStencilState failed");
	}
}

// バッファ生成ヘルパー
void ShapeRenderer::CreateWiredMesh(
	ID3D11Device* device,
	const std::vector<DirectX::XMFLOAT3>& positions,
	WiredMesh& mesh)
{
	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = static_cast<UINT>(sizeof(DirectX::XMFLOAT3) * positions.size());
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA init = {};
	init.pSysMem = positions.data();

	HRESULT hr = device->CreateBuffer(&desc, &init, mesh.vertexBuffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), L"ShapeRenderer: WiredMesh failed");
	mesh.vertexCount = static_cast<UINT>(positions.size());
}
void ShapeRenderer::CreateSolidMesh(
	ID3D11Device* device,
	const std::vector<Vertex>& vertices,
	const std::vector<uint16_t>& indices,
	SolidMesh& mesh)
{
	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * vertices.size());
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA init = {};
		init.pSysMem = vertices.data();

		HRESULT hr = device->CreateBuffer(&desc, &init, mesh.vertexBuffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), L"ShapeRenderer: SolidMesh vertex failed");
	}

	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = static_cast<UINT>(sizeof(uint16_t) * indices.size());
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA init = {};
		init.pSysMem = indices.data();

		HRESULT hr = device->CreateBuffer(&desc, &init, mesh.indexBuffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), L"ShapeRenderer: SolidMesh index failed");
	}
	mesh.indexCount = static_cast<UINT>(indices.size());
}

// 箱
void ShapeRenderer::CreateWiredBoxMesh(ID3D11Device* device)
{
	float h = 0.5f;
	std::vector<DirectX::XMFLOAT3> p =
	{
		// Bottom
		{-h,-h,-h},{h,-h,-h},  {h,-h,-h},{h,-h,h},
		{h,-h,h},{-h,-h,h},    {-h,-h,h},{-h,-h,-h},
		// Top
		{-h,h,-h},{h,h,-h},    {h,h,-h},{h,h,h},
		{h,h,h},{-h,h,h},      {-h,h,h},{-h,h,-h},
		// Verticals
		{-h,-h,-h},{-h,h,-h},  {h,-h,-h},{h,h,-h},
		{h,-h,h},{h,h,h},      {-h,-h,h},{-h,h,h},
	};
	CreateWiredMesh(device, p, wiredBoxMesh);
}
void ShapeRenderer::CreateSolidBoxMesh(ID3D11Device* device)
{
	float h = 0.5f;
	std::vector<Vertex> verts =
	{
		{{-h,-h,h},  {0,0,1}}, {{h,-h,h},  {0,0,1}},  {{h,h,h},  {0,0,1}},  {{-h,h,h}, {0,0,1}},  // +Z
		{{h,-h,-h},  {0,0,-1}},{{-h,-h,-h},{0,0,-1}}, {{-h,h,-h},{0,0,-1}}, {{h,h,-h}, {0,0,-1}}, // -Z
		{{-h,-h,-h}, {-1,0,0}},{{-h,-h,h}, {-1,0,0}}, {{-h,h,h}, {-1,0,0}}, {{-h,h,-h},{-1,0,0}}, // -X
		{{h,-h,h},   {1,0,0}}, {{h,-h,-h}, {1,0,0}},  {{h,h,-h}, {1,0,0}},  {{h,h,h},  {1,0,0}},  // +X
		{{-h,h,h},   {0,1,0}}, {{h,h,h},   {0,1,0}},  {{h,h,-h}, {0,1,0}},  {{-h,h,-h},{0,1,0}},  // +Y
		{{-h,-h,-h}, {0,-1,0}},{{h,-h,-h}, {0,-1,0}}, {{h,-h,h}, {0,-1,0}}, {{-h,-h,h},{0,-1,0}}, // -Y
	};
	std::vector<uint16_t> idxs;
	for (uint16_t f = 0; f < 6; f++)
	{
		uint16_t b = f * 4;
		idxs.push_back(b); idxs.push_back(b + 1); idxs.push_back(b + 2);
		idxs.push_back(b); idxs.push_back(b + 2); idxs.push_back(b + 3);
	}
	CreateSolidMesh(device, verts, idxs, solidBoxMesh);
}

// 球
void ShapeRenderer::CreateWiredSphereMesh(ID3D11Device* device, int subdivisions)
{
	std::vector<DirectX::XMFLOAT3> p;
	for (int i = 0; i < subdivisions; i++)
	{
		float a0 = 2.0f * PI * i / subdivisions;
		float a1 = 2.0f * PI * (i + 1) / subdivisions;
		// XY平面
		p.push_back({ cosf(a0), sinf(a0), 0 });
		p.push_back({ cosf(a1), sinf(a1), 0 });
		// XZ平面
		p.push_back({ cosf(a0), 0, sinf(a0) });
		p.push_back({ cosf(a1), 0, sinf(a1) });
		// YZ平面
		p.push_back({ 0, sinf(a0), cosf(a0) });
		p.push_back({ 0, sinf(a1), cosf(a1) });
	}
	CreateWiredMesh(device, p, wiredSphereMesh);
}
void ShapeRenderer::CreateSolidSphereMesh(ID3D11Device* device, int subdivisions)
{
	std::vector<Vertex> verts;
	std::vector<uint16_t> idxs;
	int rings = subdivisions / 2;
	int slices = subdivisions;

	for (int r = 0; r <= rings; r++)
	{
		float phi = PI * r / rings - PI * 0.5f;
		float y = sinf(phi);
		float xz = cosf(phi);
		for (int s = 0; s <= slices; s++)
		{
			float theta = 2.0f * PI * s / slices;
			float x = xz * cosf(theta);
			float z = xz * sinf(theta);
			verts.push_back({ {x,y,z},{x,y,z} });
		}
	}
	for (int r = 0; r < rings; r++)
	{
		for (int s = 0; s < slices; s++)
		{
			uint16_t a = (uint16_t)(r * (slices + 1) + s);
			uint16_t b = a + 1;
			uint16_t c = a + (uint16_t)(slices + 1);
			uint16_t d = c + 1;
			idxs.push_back(a); idxs.push_back(c); idxs.push_back(b);
			idxs.push_back(b); idxs.push_back(c); idxs.push_back(d);
		}
	}
	CreateSolidMesh(device, verts, idxs, solidSphereMesh);
}

// 半球
void ShapeRenderer::CreateWiredHalfSphereMesh(ID3D11Device* device, int subdivisions)
{
	std::vector<DirectX::XMFLOAT3> p;
	for (int i = 0; i < subdivisions; i++)
	{
		float a0 = PI * i / subdivisions;
		float a1 = PI * (i + 1) / subdivisions;
		p.push_back({ cosf(a0), sinf(a0), 0 });
		p.push_back({ cosf(a1), sinf(a1), 0 });
		p.push_back({ 0, sinf(a0), cosf(a0) });
		p.push_back({ 0, sinf(a1), cosf(a1) });
	}
	for (int i = 0; i < subdivisions; i++)
	{
		float a0 = 2.0f * PI * i / subdivisions;
		float a1 = 2.0f * PI * (i + 1) / subdivisions;
		p.push_back({ cosf(a0), 0, sinf(a0) });
		p.push_back({ cosf(a1), 0, sinf(a1) });
	}
	CreateWiredMesh(device, p, wiredHalfSphereMesh);
}
void ShapeRenderer::CreateSolidHalfSphereMesh(ID3D11Device* device, int subdivisions)
{
	std::vector<Vertex> verts;
	std::vector<uint16_t> idxs;
	int rings = subdivisions / 4;
	int slices = subdivisions;

	for (int r = 0; r <= rings; r++)
	{
		float phi = (PI * 0.5f) * r / rings;
		float y = sinf(phi);
		float xz = cosf(phi);
		for (int s = 0; s <= slices; s++)
		{
			float theta = 2.0f * PI * s / slices;
			float x = xz * cosf(theta);
			float z = xz * sinf(theta);
			verts.push_back({ {x,y,z},{x,y,z} });
		}
	}
	for (int r = 0; r < rings; r++)
	{
		for (int s = 0; s < slices; s++)
		{
			uint16_t a = (uint16_t)(r * (slices + 1) + s);
			uint16_t b = a + 1;
			uint16_t c = a + (uint16_t)(slices + 1);
			uint16_t d = c + 1;
			idxs.push_back(a); idxs.push_back(c); idxs.push_back(b);
			idxs.push_back(b); idxs.push_back(c); idxs.push_back(d);
		}
	}
	CreateSolidMesh(device, verts, idxs, solidHalfSphereMesh);
}

// 円柱
void ShapeRenderer::CreateWiredCylinderMesh(ID3D11Device* device, int subdivisions)
{
	std::vector<DirectX::XMFLOAT3> p;
	for (int i = 0; i < subdivisions; i++)
	{
		float a0 = 2.0f * PI * i / subdivisions;
		float a1 = 2.0f * PI * (i + 1) / subdivisions;
		// 上リング
		p.push_back({ cosf(a0),  0.5f, sinf(a0) });
		p.push_back({ cosf(a1),  0.5f, sinf(a1) });
		// 下リング
		p.push_back({ cosf(a0), -0.5f, sinf(a0) });
		p.push_back({ cosf(a1), -0.5f, sinf(a1) });
	}
	// 4本縦ライン
	for (int i = 0; i < 4; i++)
	{
		float a = 2.0f * PI * i / 4;
		float x = cosf(a), z = sinf(a);
		p.push_back({ x,  0.5f, z });
		p.push_back({ x, -0.5f, z });
	}
	CreateWiredMesh(device, p, wiredCylinderMesh);
}
void ShapeRenderer::CreateSolidCylinderMesh(ID3D11Device* device, int subdivisions)
{
	std::vector<Vertex> verts;
	std::vector<uint16_t> idxs;

	// 側面のみ
	for (int i = 0; i <= subdivisions; i++)
	{
		float theta = 2.0f * PI * i / subdivisions;
		float x = cosf(theta), z = sinf(theta);
		verts.push_back({ {x,  0.5f, z}, {x, 0, z} });
		verts.push_back({ {x, -0.5f, z}, {x, 0, z} });
	}
	for (int i = 0; i < subdivisions; i++)
	{
		uint16_t t0 = (uint16_t)(i * 2);
		uint16_t b0 = t0 + 1, t1 = t0 + 2, b1 = t0 + 3;
		idxs.push_back(t0); idxs.push_back(t1); idxs.push_back(b0);
		idxs.push_back(t1); idxs.push_back(b1); idxs.push_back(b0);
	}
	CreateSolidMesh(device, verts, idxs, solidCylinderMesh);
}
