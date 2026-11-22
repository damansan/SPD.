#pragma once
#include <memory>
#include <string>
#include <filesystem>

#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#include "tiny_gltf.h"
#include "GLTFData.h"

class GLTFImporter
{
public:
    // GLTFファイルを読み込んでGLTFDataを返す
    static std::shared_ptr<GLTFData> Load(const std::string& filename);

private:
    // ノード読み込み
    static void LoadNodes(
        GLTFData& data,
        const tinygltf::Model& gltfModel
    );

    // メッシュ読み込み
    static void LoadMeshes(
        GLTFData& data,
        const tinygltf::Model& gltfModel
    );

    // マテリアル読み込み
    static void LoadMaterials(
        GLTFData& data,
        const tinygltf::Model& gltfModel,
        const std::filesystem::path& filepath
    );

    // テクスチャ読み込み
    static void LoadTextures(
        GLTFData& data,
        const tinygltf::Model& gltfModel,
        const std::filesystem::path& filepath
    );

    // アニメーション読み込み
    static void LoadAnimations(
        GLTFData& data,
        const tinygltf::Model& gltfModel
    );

    // スキニング判定
    static bool CheckHasSkin(const tinygltf::Model& gltfModel);

    // 座標系変換ヘルパー
    static DirectX::XMFLOAT3   ToXMFLOAT3(const std::vector<double>& v);
    static DirectX::XMFLOAT4   ToXMFLOAT4(const std::vector<double>& v);
    static DirectX::XMFLOAT4X4 ToXMFLOAT4X4(const std::vector<double>& v);

    // タンジェント計算
    static void ComputeTangents(
        std::vector<GLTFData::Vertex>& vertices,
        const std::vector<uint32_t>& indices
    );

    // グローバルトランスフォーム計算
    static void CalcGlobalTransform(
        std::vector<GLTFData::Node>& nodes,
        int nodeIndex,
        const DirectX::XMFLOAT4X4& parentTransform
    );
};