#pragma once

#include <string>
#include <vector>
#include <DirectXMath.h>
#include <wrl.h>
#include <d3d11.h>

class AnimationEditData;

class Model
{
public:

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
        int nodeIndex;
        DirectX::XMFLOAT4X4 offsetTransform;
    };

    // ノード
    struct Node
    {
        std::string         name;
        int                 parentIndex = -1;
        std::vector<int>    childIndices;
        DirectX::XMFLOAT3   position = { 0, 0, 0 };
        DirectX::XMFLOAT4   rotation = { 0, 0, 0, 1 };
        DirectX::XMFLOAT3   scale = { 1, 1, 1 };
        DirectX::XMFLOAT4X4 localTransform = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
        DirectX::XMFLOAT4X4 globalTransform = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    };

    // マテリアル
    struct Material
    {
        std::string       name;
        DirectX::XMFLOAT4 baseColor = { 1, 1, 1, 1 };
        float             roughnessFactor = 1.0f;
        float             metallicFactor = 0.0f;
        DirectX::XMFLOAT3 emissiveFactor = { 0, 0, 0 }; // 発光の係数

        // 外部ファイル参照テクスチャ用パス（シリアライズ対象）
        std::string baseTextureFileName;
        std::string normalTextureFileName;
        std::string metallicRoughnessTextureFileName;
        std::string occlusionTextureFileName;
        std::string emissiveTextureFileName;

        // 埋め込みテクスチャ用バイナリデータ（シリアライズ対象）
        std::vector<uint8_t> baseMapData;
        std::vector<uint8_t> normalMapData;
        std::vector<uint8_t> metallicRoughnessMapData;
        std::vector<uint8_t> occlusionMapData;
        std::vector<uint8_t> emissiveMapData;

        // GPUリソース(シリアライズ対象外)
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> baseMap;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalMap;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metallicRoughnessMap;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> occlusionMap;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> emissiveMap;

        enum class AlphaMode { Opaque, Blend, Mask };
        AlphaMode alphaMode = AlphaMode::Opaque;
        float     alphaCutoff = 0.5f;
    };

    // メッシュ
    struct Mesh
    {
        std::vector<Vertex>   vertices;
        std::vector<uint32_t> indices;
        std::vector<Bone>     bones;
        int      nodeIndex = 0;
        int      materialIndex = 0;
        Material* material = nullptr;

        Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
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

        // 焼き込み後の時刻と、生データの時刻の対応表
        std::vector<float>    bakedTimes;
        std::vector<float>    rawTimes;

        // 焼き込み後から生データに
        float ToRawTime(float bakedTime) const;
        // 生データから焼き込み後に
        float ToBakedTime(float rawTime) const;

        // 1フレームの秒数
        float CalcFrameStep() const;
    };

    // 入力レイアウト
    static const std::vector<D3D11_INPUT_ELEMENT_DESC> InputElementDescs;

    // スキニングの有無
    bool HasSkin() const { return hasSkin; }


    // データアクセサー
    const std::vector<Node>& GetNodes()      const { return nodes; }
    const std::vector<Mesh>& GetMeshes()     const { return meshes; }
    const std::vector<Material>& GetMaterials()  const { return materials; }
    const std::vector<Animation>& GetAnimations() const { return animations; }       // 編集済み
    const std::vector<Animation>& GetRawAnimations() const { return rawAnimations; } // 生データ
    
    // 名前からクリップを探す
    const Animation* FindAnimation(const std::string& name) const;        // 編集済み
    const Animation* FindRawAnimation(const std::string& name) const;     // 生データ
    const Animation* FindPreviewAnimation(const std::string& name) const; // プレビュー用

	// 編集設定をプレビュー用に適用する
    void ApplyAnimationEditPreview(const AnimationEditData& editData);
    // プレビュー用を消す
    void ClearAnimationPreview();

    // 再生に使うクリップの並び
    const std::vector<Animation>& GetPlaybackAnimations(bool usePreview) const;

    // 編集設定を焼き込む
    void ApplyAnimationEdit(const AnimationEditData& editData);
    // 焼き込みに使ったルートノード名と番号
    const std::string& GetRootNodeName() const { return rootNodeName; }
    int GetRootNodeIndex() const { return rootNodeIndex; }

    std::vector<Node>& GetNodes() { return nodes; }
    std::vector<Mesh>& GetMeshes() { return meshes; }
    std::vector<Material>& GetMaterials() { return materials; }


    // バウンディングボックス
    struct BoundingBox
    {
        DirectX::XMFLOAT3 min = { FLT_MAX,  FLT_MAX,  FLT_MAX };
        DirectX::XMFLOAT3 max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

        // 中心座標取得
        DirectX::XMFLOAT3 GetCenter() const
        {
            return {
                (min.x + max.x) * 0.5f,
                (min.y + max.y) * 0.5f,
                (min.z + max.z) * 0.5f
            };
        }

        // 半径取得（球体近似）
        float GetRadius() const
        {
            float dx = (max.x - min.x) * 0.5f;
            float dy = (max.y - min.y) * 0.5f;
            float dz = (max.z - min.z) * 0.5f;
            return sqrtf(dx * dx + dy * dy + dz * dz);
        }
    };
    // バウンディングボックス取得
    const BoundingBox& GetBoundingBox() const { return boundingBox; }

private:
    friend class GLTFImporter;
    friend class GLTFConverter;
    friend class ModelSerializer;

    // 生データがまだ控えられていなければ、今のアニメーションを控える
    void EnsureRawAnimations();

    std::vector<Node>      nodes;
    std::vector<Mesh>      meshes;
    std::vector<Material>  materials;
    std::vector<Animation> animations;        // 編集済み
    std::vector<Animation> rawAnimations;     // 生データ
    std::vector<Animation> previewAnimations; // プレビュー用
    std::string            rootNodeName;       // 焼き込みに使ったルートノード名
    int                    rootNodeIndex = -1; // その番号

    bool                   hasSkin = false;

    BoundingBox boundingBox;
};