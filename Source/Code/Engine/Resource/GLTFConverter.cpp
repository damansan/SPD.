#include "GLTFConverter.h"
#include "GpuResourceUtils.h"
#include "Engine/Utility/misc.h"

#include <fstream>
#include <vector>

// GLTFData を受け取り Model を生成して返す
std::shared_ptr<Model> GLTFConverter::Convert(
	ID3D11Device* device,
	const std::shared_ptr<GLTFData>& data)
{
	auto model = std::make_shared<Model>();
	model->hasSkin = data->hasSkin;

	ConvertNodes(*model, *data);
	ConvertMeshes(*model, *data, device);
	ConvertMaterials(*model, *data, device);
	ConvertAnimations(*model, *data);

	return model;
}

// .cereal から復元した後にGPUリソースだけ再生成する
void GLTFConverter::CreateGpuResources(ID3D11Device* device, Model& model)
{
	// メッシュのGPUバッファ再生成
	for (auto& mesh : model.meshes)
	{
		// 頂点バッファ
		{
			D3D11_BUFFER_DESC desc{};
			desc.ByteWidth = static_cast<UINT>(
				sizeof(Model::Vertex) * mesh.vertices.size());
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			D3D11_SUBRESOURCE_DATA sub{};
			sub.pSysMem = mesh.vertices.data();
			HRESULT hr = device->CreateBuffer(
				&desc, &sub, mesh.vertexBuffer.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		}
		// インデックスバッファ
		{
			D3D11_BUFFER_DESC desc{};
			desc.ByteWidth = static_cast<UINT>(
				sizeof(uint32_t) * mesh.indices.size());
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			D3D11_SUBRESOURCE_DATA sub{};
			sub.pSysMem = mesh.indices.data();
			HRESULT hr = device->CreateBuffer(
				&desc, &sub, mesh.indexBuffer.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		}
	}

	// マテリアルのテクスチャ再生成
	for (auto& mat : model.materials)
	{
		// colorSpace : 色のテクスチャ（Base・Emissive）なら sRGB、データ（Normal など）なら Linear
		auto loadTex = [&](
			const std::string& filepath,
			const std::vector<uint8_t>& pixels,
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv,
			DWORD dummyColor,
			TextureColorSpace colorSpace)
			{
				if (!filepath.empty())
				{
					// 外部ファイル → DDSキャッシュ優先でロード
					GpuResourceUtils::LoadTextureWithDDSCache(device, filepath, srv, colorSpace);
				}
				else if (!pixels.empty())
				{
					// 埋め込みバイナリ → そのままロード（DDSキャッシュ対象外）
					GpuResourceUtils::LoadTexture(device, pixels.data(), pixels.size(), srv, colorSpace);
				}
				else
				{
					GpuResourceUtils::CreateDummyTexture(device, srv, dummyColor, 1);
				}
			};
		loadTex(mat.baseTextureFileName, mat.baseMapData, mat.baseMap, 0xFFFFFFFF, TextureColorSpace::sRGB);
		loadTex(mat.normalTextureFileName, mat.normalMapData, mat.normalMap, 0xFFFF8080, TextureColorSpace::Linear);
		loadTex(mat.metallicRoughnessTextureFileName, mat.metallicRoughnessMapData, mat.metallicRoughnessMap, 0xFFFFFFFF, TextureColorSpace::Linear);
		loadTex(mat.occlusionTextureFileName, mat.occlusionMapData, mat.occlusionMap, 0xFFFFFFFF, TextureColorSpace::Linear);
		loadTex(mat.emissiveTextureFileName, mat.emissiveMapData, mat.emissiveMap, 0xFFFFFFFF, TextureColorSpace::sRGB); // テクスチャがなければ白（発光 = 係数）
	}

	// material ポインタの再接続
	for (auto& mesh : model.meshes)
	{
		if (mesh.materialIndex >= 0 &&
			mesh.materialIndex < (int)model.materials.size())
		{
			mesh.material = &model.materials[mesh.materialIndex];
		}
	}
}

// ノード変換
void GLTFConverter::ConvertNodes(
	Model& model,
	const GLTFData& data)
{
	model.nodes.reserve(data.nodes.size());

	for (const auto& src : data.nodes)
	{
		Model::Node dst;
		dst.name = src.name;
		dst.parentIndex = src.parentIndex;
		dst.childIndices = src.childIndices;
		dst.position = src.position;
		dst.rotation = src.rotation;
		dst.scale = src.scale;
		dst.localTransform = src.localTransform;
		dst.globalTransform = src.globalTransform;

		model.nodes.push_back(std::move(dst));
	}
}

// メッシュ変換
void GLTFConverter::ConvertMeshes(
	Model& model,
	const GLTFData& data,
	ID3D11Device* device)
{
	model.meshes.reserve(data.meshes.size());

	for (const auto& src : data.meshes)
	{
		Model::Mesh dst;
		dst.nodeIndex = src.nodeIndex;
		dst.materialIndex = src.materialIndex;

		// 頂点データをGLTFData::VertexからModel::Vertexへコピー
		dst.vertices.reserve(src.vertices.size());
		for (const auto& v : src.vertices)
		{
			Model::Vertex mv;
			mv.position = v.position;
			mv.normal = v.normal;
			mv.tangent = v.tangent;
			mv.texcoord = v.texcoord;
			mv.boneWeight = v.boneWeight;
			mv.boneIndex = v.boneIndex;
			dst.vertices.push_back(mv);
		}

		// インデックスデータコピー
		dst.indices = src.indices;

		// ボーンデータコピー
		dst.bones.reserve(src.bones.size());
		for (const auto& b : src.bones)
		{
			Model::Bone mb;
			mb.nodeIndex = b.nodeIndex;
			mb.offsetTransform = b.offsetTransform;
			dst.bones.push_back(mb);
		}

		// 頂点バッファ生成
		{
			D3D11_BUFFER_DESC desc{};
			desc.ByteWidth = static_cast<UINT>(
				sizeof(Model::Vertex) * dst.vertices.size());
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

			D3D11_SUBRESOURCE_DATA sub{};
			sub.pSysMem = dst.vertices.data();

			HRESULT hr = device->CreateBuffer(
				&desc, &sub, dst.vertexBuffer.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		}

		// インデックスバッファ生成
		{
			D3D11_BUFFER_DESC desc{};
			desc.ByteWidth = static_cast<UINT>(
				sizeof(uint32_t) * dst.indices.size());
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

			D3D11_SUBRESOURCE_DATA sub{};
			sub.pSysMem = dst.indices.data();

			HRESULT hr = device->CreateBuffer(
				&desc, &sub, dst.indexBuffer.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		}

		model.meshes.push_back(std::move(dst));
	}
	// 全メッシュの頂点からバウンディングボックスを計算
	for (const auto& mesh : model.meshes)
	{
		for (const auto& v : mesh.vertices)
		{
			model.boundingBox.min.x = (std::min)(model.boundingBox.min.x, v.position.x);
			model.boundingBox.min.y = (std::min)(model.boundingBox.min.y, v.position.y);
			model.boundingBox.min.z = (std::min)(model.boundingBox.min.z, v.position.z);

			model.boundingBox.max.x = (std::max)(model.boundingBox.max.x, v.position.x);
			model.boundingBox.max.y = (std::max)(model.boundingBox.max.y, v.position.y);
			model.boundingBox.max.z = (std::max)(model.boundingBox.max.z, v.position.z);
		}
	}
}

// マテリアル変換
void GLTFConverter::ConvertMaterials(
	Model& model,
	const GLTFData& data,
	ID3D11Device* device)
{
	model.materials.reserve(data.materials.size());

	// colorSpace : 色のテクスチャ（Base・Emissive）なら sRGB、データ（Normal など）なら Linear
	auto convertTexture = [&](
		int texIndex,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv,
		DWORD dummyColor,
		TextureColorSpace colorSpace)
		{
			if (texIndex < 0 ||
				texIndex >= (int)data.textures.size())
			{
				GpuResourceUtils::CreateDummyTexture(device, srv, dummyColor, 1);
				return;
			}

			const auto& texData = data.textures[texIndex];

			if (!texData.filepath.empty())
			{
				// 外部ファイルから読む
				// DDSキャッシュ優先でロード
				GpuResourceUtils::LoadTextureWithDDSCache(device, texData.filepath, srv, colorSpace);
			}
			else if (!texData.pixels.empty())
			{
				// 埋め込みバイナリから読む(そのままロード(DDSキャッシュ対象外))
				GpuResourceUtils::LoadTexture(device, texData.pixels.data(), texData.pixels.size(), srv, colorSpace);
			}
			else
			{
				GpuResourceUtils::CreateDummyTexture(device, srv, dummyColor, 1);
			}
		};

	for (const auto& src : data.materials)
	{
		Model::Material dst;
		dst.name = src.name;
		dst.baseColor = src.baseColor;
		dst.roughnessFactor = src.roughnessFactor;
		dst.metallicFactor = src.metallicFactor;
		dst.emissiveFactor = src.emissiveFactor;
		dst.alphaMode = static_cast<Model::Material::AlphaMode>(src.alphaMode);
		dst.alphaCutoff = src.alphaCutoff;

		// 外部ファイルならパスを、埋め込みならバイナリデータを保存
		auto saveTexture = [&](
			int texIndex,
			std::string& outPath,
			std::vector<uint8_t>& outPixels)
			{
				if (texIndex < 0 || texIndex >= (int)data.textures.size()) return;
				const auto& texData = data.textures[texIndex];

				if (!texData.filepath.empty())
				{
					// 外部ファイル参照ならパスだけ保存
					outPath = texData.filepath;
				}
				else if (!texData.pixels.empty())
				{
					// 埋め込みならバイナリデータを保存
					outPixels = texData.pixels;
				}
			};

		saveTexture(src.baseMapIndex, dst.baseTextureFileName, dst.baseMapData);
		saveTexture(src.normalMapIndex, dst.normalTextureFileName, dst.normalMapData);
		saveTexture(src.metallicRoughnessMapIndex, dst.metallicRoughnessTextureFileName, dst.metallicRoughnessMapData);
		saveTexture(src.occlusionMapIndex, dst.occlusionTextureFileName, dst.occlusionMapData);
		saveTexture(src.emissiveMapIndex, dst.emissiveTextureFileName, dst.emissiveMapData);

		// テクスチャ生成（初回ロード時）
		convertTexture(src.baseMapIndex, dst.baseMap, 0xFFFFFFFF, TextureColorSpace::sRGB);
		convertTexture(src.normalMapIndex, dst.normalMap, 0xFFFF8080, TextureColorSpace::Linear);
		convertTexture(src.metallicRoughnessMapIndex, dst.metallicRoughnessMap, 0xFFFFFFFF, TextureColorSpace::Linear);
		convertTexture(src.occlusionMapIndex, dst.occlusionMap, 0xFFFFFFFF, TextureColorSpace::Linear);
		convertTexture(src.emissiveMapIndex, dst.emissiveMap, 0xFFFFFFFF, TextureColorSpace::sRGB); // テクスチャがなければ白

		model.materials.push_back(std::move(dst));
	}
	// メッシュとマテリアルを紐付け
	for (auto& mesh : model.meshes)
	{
		if (mesh.materialIndex >= 0 &&
			mesh.materialIndex < (int)model.materials.size())
		{
			mesh.material = &model.materials[mesh.materialIndex];
		}
	}
}

// アニメーション変換
void GLTFConverter::ConvertAnimations(
	Model& model,
	const GLTFData& data)
{
	model.animations.reserve(data.animations.size());

	for (const auto& src : data.animations)
	{
		Model::Animation dst;
		dst.name = src.name;
		dst.secondsLength = src.secondsLength;

		dst.nodeAnims.reserve(src.nodeAnims.size());
		for (const auto& srcNode : src.nodeAnims)
		{
			Model::NodeAnim dstNode;

			// positionキーフレーム
			dstNode.positionKeyframes.reserve(
				srcNode.positionKeyframes.size());
			for (const auto& kf : srcNode.positionKeyframes)
			{
				Model::VectorKeyframe dkf;
				dkf.seconds = kf.seconds;
				dkf.value = kf.value;
				dstNode.positionKeyframes.push_back(dkf);
			}

			// rotationキーフレーム
			dstNode.rotationKeyframes.reserve(
				srcNode.rotationKeyframes.size());
			for (const auto& kf : srcNode.rotationKeyframes)
			{
				Model::QuaternionKeyframe dkf;
				dkf.seconds = kf.seconds;
				dkf.value = kf.value;
				dstNode.rotationKeyframes.push_back(dkf);
			}

			// scaleキーフレーム
			dstNode.scaleKeyframes.reserve(
				srcNode.scaleKeyframes.size());
			for (const auto& kf : srcNode.scaleKeyframes)
			{
				Model::VectorKeyframe dkf;
				dkf.seconds = kf.seconds;
				dkf.value = kf.value;
				dstNode.scaleKeyframes.push_back(dkf);
			}

			dst.nodeAnims.push_back(std::move(dstNode));
		}

		model.animations.push_back(std::move(dst));
	}
}
