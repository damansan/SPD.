#include "ModelSerializer.h"
#include "Model.h"
#include "Engine/Utility/CerealHelper.h"

#include <fstream>
#include <filesystem>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>


// Model内部構造体のserialize定義

template<class T>
void serialize(T& archive, Model::Vertex& v)
{
	archive(v.position, v.normal, v.tangent, v.texcoord, v.boneWeight, v.boneIndex);
}

template<class T>
void serialize(T& archive, Model::Bone& b)
{
	archive(b.nodeIndex, b.offsetTransform);
}

template<class T>
void serialize(T& archive, Model::Node& n)
{
	archive(n.name, n.parentIndex, n.childIndices,
		n.position, n.rotation, n.scale,
		n.localTransform, n.globalTransform);
}

template<class T>
void serialize(T& archive, Model::Material& m)
{
	int alphaMode = static_cast<int>(m.alphaMode);
	archive(
		m.name,
		m.baseColor,
		m.roughnessFactor,
		m.metallicFactor,
		m.emissiveFactor,
		alphaMode,
		m.alphaCutoff,
		// 外部ファイル参照用パス
		m.baseTextureFileName,
		m.normalTextureFileName,
		m.metallicRoughnessTextureFileName,
		m.occlusionTextureFileName,
		m.emissiveTextureFileName,
		// 埋め込みテクスチャ用バイナリ
		m.baseMapData,
		m.normalMapData,
		m.metallicRoughnessMapData,
		m.occlusionMapData,
		m.emissiveMapData
	);
	m.alphaMode = static_cast<Model::Material::AlphaMode>(alphaMode);
}

template<class T>
void serialize(T& archive, Model::Mesh& m)
{
	// GPUリソース（Buffer）は保存しない
	archive(m.vertices, m.indices, m.bones, m.nodeIndex, m.materialIndex);
}

template<class T>
void serialize(T& archive, Model::VectorKeyframe& k)
{
	archive(k.seconds, k.value);
}

template<class T>
void serialize(T& archive, Model::QuaternionKeyframe& k)
{
	archive(k.seconds, k.value);
}

template<class T>
void serialize(T& archive, Model::NodeAnim& n)
{
	archive(n.positionKeyframes, n.rotationKeyframes, n.scaleKeyframes);
}

template<class T>
void serialize(T& archive, Model::Animation& a)
{
	archive(a.name, a.secondsLength, a.nodeAnims);
}

template<class T>
void serialize(T& archive, Model::BoundingBox& b)
{
	archive(b.min, b.max);
}


// 明示的インスタンス化
#define INSTANTIATE(Type) \
    template void serialize(cereal::BinaryOutputArchive&, Type&); \
    template void serialize(cereal::BinaryInputArchive&,  Type&);

INSTANTIATE(Model::Vertex)
INSTANTIATE(Model::Bone)
INSTANTIATE(Model::Node)
INSTANTIATE(Model::Material)
INSTANTIATE(Model::Mesh)
INSTANTIATE(Model::VectorKeyframe)
INSTANTIATE(Model::QuaternionKeyframe)
INSTANTIATE(Model::NodeAnim)
INSTANTIATE(Model::Animation)
INSTANTIATE(Model::BoundingBox)

#undef INSTANTIATE



void ModelSerializer::Save(const Model& model, const std::string& filepath)
{
	std::ofstream ofs(filepath, std::ios::binary);
	cereal::BinaryOutputArchive archive(ofs);
	archive(
		model.nodes,
		model.meshes,
		model.materials,
		model.animations,
		model.hasSkin,
		model.boundingBox
	);
}

void ModelSerializer::Load(Model& model, const std::string& filepath)
{
	std::ifstream ifs(filepath, std::ios::binary);
	cereal::BinaryInputArchive archive(ifs);
	archive(
		model.nodes,
		model.meshes,
		model.materials,
		model.animations,
		model.hasSkin,
		model.boundingBox
	);
}

bool ModelSerializer::Exists(const std::string& gltfPath)
{
	return std::filesystem::exists(ToCerealPath(gltfPath));
}

std::string ModelSerializer::ToCerealPath(const std::string& gltfPath)
{
	std::filesystem::path path(gltfPath);
	path.replace_extension(".cereal");
	return path.string();
}