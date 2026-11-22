#pragma once
#include <memory>
#include <vector>
#include <DirectXMath.h>
#include "Engine/Resource/Model.h"

// スキニングメッシュ用コンポーネント
struct SkinnedMeshComponent
{
    // モデルデータ
    std::shared_ptr<Model> model; 
    // モデルの読み込み元パス
    std::string modelPath;

    // メッシュごとのボーン行列
    std::vector<std::vector<DirectX::XMFLOAT4X4>> boneTransforms;

    // ノードごとのグローバル行列
    std::vector<DirectX::XMFLOAT4X4> nodeGlobalTransforms;


    std::vector<DirectX::XMFLOAT3> localPositions;
    std::vector<DirectX::XMFLOAT4> localRotations;
    std::vector<DirectX::XMFLOAT3> localScales;
};