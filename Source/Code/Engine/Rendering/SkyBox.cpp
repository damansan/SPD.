#include "SkyBox.h"
#include "Engine/Resource/GpuResourceUtils.h"
#include "Engine/Utility/misc.h"
#include <filesystem>

void SkyBox::Initialize(
	ID3D11Device* device,
	const std::string& folderPath) 
{
	// 末尾にスラッシュがなければ追加
	std::string folder = folderPath;
	if (!folder.empty() && folder.back() != '/' && folder.back() != '\\')
		folder += '/';

	// キューブマップ読み込み
	GpuResourceUtils::LoadDDSTexture(
		device, folder + "specular_pmrem.dds", skyTexture);

	GpuResourceUtils::LoadDDSTexture(
		device, folder + "diffuse_iem.dds", diffuseIem);

	GpuResourceUtils::LoadDDSTexture(
		device, folder + "lut_ggx.dds", lutGgx);

	// スカイボックス描画用（skybox.ddsがあれば使う、無かったらspecular_pmrem.ddsを使う）
	if (std::filesystem::exists(folder + "skybox.dds"))
	{
		GpuResourceUtils::LoadDDSTexture(
			device, folder + "skybox.dds", skyBoxTexture);
	}
	else
	{
		skyBoxTexture = skyTexture;
	}

	// 入力レイアウト（POSITIONのみ）
	const D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
		  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// 頂点シェーダー読み込み
	GpuResourceUtils::LoadVertexShader(
		device,
		"Data/Shader/SkyBox_VS.cso",
		layout, 1,
		inputLayout,
		vertexShader
	);

	// ピクセルシェーダー読み込み
	GpuResourceUtils::LoadPixelShader(
		device,
		"Data/Shader/SkyBox_PS.cso",
		pixelShader
	);

	// 定数バッファ生成
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbSkyBox),
		cbSkyBox
	);

	// サンプラーステート生成
	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	//samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	//samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	//samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&samplerDesc, samplerState.GetAddressOf());

	// 深度ステンシルステート生成
	D3D11_DEPTH_STENCIL_DESC dsDesc{};
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // 書き込みOFF
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; // LESS_EQUALで1.0を通す
	device->CreateDepthStencilState(&dsDesc, depthStencilState.GetAddressOf());

	// 単位立方体の頂点データ
	const DirectX::XMFLOAT3 vertices[] =
	{
		{ -1.0f,  1.0f, -1.0f },
		{  1.0f,  1.0f, -1.0f },
		{  1.0f,  1.0f,  1.0f },
		{ -1.0f,  1.0f,  1.0f },
		{ -1.0f, -1.0f, -1.0f },
		{  1.0f, -1.0f, -1.0f },
		{  1.0f, -1.0f,  1.0f },
		{ -1.0f, -1.0f,  1.0f },
	};

	// インデックスデータ
	const UINT indices[] =
	{
		0, 1, 2,  0, 2, 3, // 上面
		4, 6, 5,  4, 7, 6, // 下面
		0, 4, 5,  0, 5, 1, // 前面
		2, 6, 7,  2, 7, 3, // 後面
		0, 3, 7,  0, 7, 4, // 左面
		1, 5, 6,  1, 6, 2, // 右面
	};
	indexCount = ARRAYSIZE(indices);

	// 頂点バッファ生成
	D3D11_BUFFER_DESC vbDesc{};
	vbDesc.ByteWidth = sizeof(vertices);
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vbData{};
	vbData.pSysMem = vertices;
	device->CreateBuffer(&vbDesc, &vbData, vertexBuffer.GetAddressOf());

	// インデックスバッファ生成
	D3D11_BUFFER_DESC ibDesc{};
	ibDesc.ByteWidth = sizeof(indices);
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA ibData{};
	ibData.pSysMem = indices;
	device->CreateBuffer(&ibDesc, &ibData, indexBuffer.GetAddressOf());
}

void SkyBox::Render(
	ID3D11DeviceContext* dc,
	const DirectX::XMFLOAT4X4& view,
	const DirectX::XMFLOAT4X4& projection,
	const DirectX::XMFLOAT3& cameraPosition)
{
	if (!enabled) return;
	if (!skyTexture) return;

	// 定数バッファ更新
	CbSkyBox cbData{};
	cbData.view = view;
	cbData.projection = projection;
	cbData.cameraPosition = { cameraPosition.x, cameraPosition.y,cameraPosition.z, 0.0f };
	dc->UpdateSubresource(cbSkyBox.Get(), 0, nullptr, &cbData, 0, 0);

	// シェーダー設定
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	dc->PSSetShader(pixelShader.Get(), nullptr, 0);
	dc->IASetInputLayout(inputLayout.Get());

	// 定数バッファ設定
	dc->VSSetConstantBuffers(0, 1, cbSkyBox.GetAddressOf());

	// テクスチャ設定（t10スロット）
	//dc->PSSetShaderResources(10, 1, skyTexture.GetAddressOf());
	dc->PSSetShaderResources(10, 1, skyBoxTexture.GetAddressOf());
	dc->PSSetSamplers(0, 1, samplerState.GetAddressOf());

	// 深度ステート設定（書き込みOFF）
	dc->OMSetDepthStencilState(depthStencilState.Get(), 0);

	// 頂点・インデックスバッファ設定
	UINT stride = sizeof(DirectX::XMFLOAT3);
	UINT offset = 0;
	dc->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	dc->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 描画
	dc->DrawIndexed(indexCount, 0, 0);
}