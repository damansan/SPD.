#include "PrimitiveRenderer.h"
#include "Engine/Resource/GpuResourceUtils.h"

PrimitiveRenderer::PrimitiveRenderer(ID3D11Device* device)
{
	D3D11_INPUT_ELEMENT_DESC inputElementDesc[]
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	GpuResourceUtils::LoadVertexShader(
		device,
		"Data/Shader/Debug_VS.cso",
		inputElementDesc,
		_countof(inputElementDesc),
		inputLayout,
		vertexShader);

	GpuResourceUtils::LoadPixelShader(
		device,
		"Data/Shader/Debug_PS.cso",
		pixelShader);

	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbScene),
		constantBuffer);

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = sizeof(Vertex) * VertexCapacity;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	HRESULT hr = device->CreateBuffer(&desc, nullptr, vertexBuffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), L"PrimitiveRenderer: vertexBuffer failed");
}

void PrimitiveRenderer::AddVertex(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& color)
{
	Vertex& v = vertices.emplace_back();
	v.position = position;
	v.color = color;
}

void PrimitiveRenderer::Render(
	ID3D11DeviceContext* dc,
	const DirectX::XMFLOAT4X4& view,
	const DirectX::XMFLOAT4X4& projection,
	D3D11_PRIMITIVE_TOPOLOGY topology)
{
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	dc->PSSetShader(pixelShader.Get(), nullptr, 0);
	dc->IASetInputLayout(inputLayout.Get());
	dc->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());

	// viewProjection更新
	DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&view);
	DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&projection);
	CbScene cbScene;
	DirectX::XMStoreFloat4x4(&cbScene.viewProjection, V * P);
	dc->UpdateSubresource(constantBuffer.Get(), 0, nullptr, &cbScene, 0, 0);

	if (vertices.empty()) return;

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	dc->IASetPrimitiveTopology(topology);
	dc->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);

	UINT total = static_cast<UINT>(vertices.size());
	UINT start = 0;
	UINT count = (total < VertexCapacity) ? total : VertexCapacity;

	while (start < total)
	{
		D3D11_MAPPED_SUBRESOURCE mapped;
		HRESULT hr = dc->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		_ASSERT_EXPR(SUCCEEDED(hr), L"PrimitiveRenderer: Map failed");
		memcpy(mapped.pData, &vertices[start], sizeof(Vertex) * count);
		dc->Unmap(vertexBuffer.Get(), 0);
		dc->Draw(count, 0);

		start += count;
		if ((start + count) > total) count = total - start;
	}

	vertices.clear();
}