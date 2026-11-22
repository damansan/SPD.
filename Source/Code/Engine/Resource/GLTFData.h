#pragma once
#include <string>
#include <vector>
#include <DirectXMath.h>


// GLTFファイルから読み込んだデータ
struct GLTFData
{
    // 頂点データ
    struct Vertex
    {
        DirectX::XMFLOAT3 position = { 0, 0, 0 };
        DirectX::XMFLOAT3 normal = { 0, 0, 0 };
        DirectX::XMFLOAT4 tangent = { 0, 0, 0, 1 };
        DirectX::XMFLOAT2 texcoord = { 0, 0 };
        DirectX::XMFLOAT4 boneWeight = { 1, 0, 0, 0 };
        DirectX::XMUINT4  boneIndex = { 0, 0, 0, 0 };
    };

    // ボーン
    struct Bone
    {
        int                 nodeIndex;
        DirectX::XMFLOAT4X4 offsetTransform;
    };

    // ノード
    struct Node
    {
        std::string          name;
        int                  parentIndex = -1;
        std::vector<int>     childIndices;
        DirectX::XMFLOAT3    position = { 0, 0, 0 };
        DirectX::XMFLOAT4    rotation = { 0, 0, 0, 1 };
        DirectX::XMFLOAT3    scale = { 1, 1, 1 };
        DirectX::XMFLOAT4X4  localTransform = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
        DirectX::XMFLOAT4X4  globalTransform = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    };

    // テクスチャデータ
    struct TextureData
    {
        std::vector<uint8_t> pixels; // バイナリデータ
        std::string          filepath; // 外部ファイルの場合のパス

    };

    // マテリアルデータ
    struct MaterialData
    {
        std::string          name;
        DirectX::XMFLOAT4    baseColor = { 1, 1, 1, 1 };
        float                roughnessFactor = 1.0f;
        float                metallicFactor = 0.0f;
        DirectX::XMFLOAT3    emissiveFactor = { 0, 0, 0 };

        // テクスチャインデックス（-1 は未使用）
        int baseMapIndex = -1;
        int normalMapIndex = -1;
        int metallicRoughnessMapIndex = -1;
        int occlusionMapIndex = -1;
        int emissiveMapIndex = -1;

        enum class AlphaMode { Opaque, Blend, Mask };
        AlphaMode alphaMode = AlphaMode::Opaque;
        float     alphaCutoff = 0.5f;
    };

    // メッシュデータ
    struct MeshData
    {
        std::vector<Vertex>   vertices;
        std::vector<uint32_t> indices;
        std::vector<Bone>     bones;
        int nodeIndex = 0;
        int materialIndex = 0;
    };

    // アニメーションキーフレーム
    struct VectorKeyframe
    {
        float               seconds;
        DirectX::XMFLOAT3   value;
    };
    struct QuaternionKeyframe
    {
        float               seconds;
        DirectX::XMFLOAT4   value;
    };

    // ノードアニメーション
    struct NodeAnim
    {
        std::vector<VectorKeyframe>     positionKeyframes;
        std::vector<QuaternionKeyframe> rotationKeyframes;
        std::vector<VectorKeyframe>     scaleKeyframes;
    };

    // アニメーション
    struct Animation
    {
        std::string           name;
        float                 secondsLength;
        std::vector<NodeAnim> nodeAnims;
    };

    // データ本体
    std::vector<Node>        nodes;
    std::vector<MeshData>    meshes;
    std::vector<MaterialData> materials;
    std::vector<TextureData> textures;   // 全テクスチャをここに集約
    std::vector<Animation>   animations;
    bool                     hasSkin = false;
};