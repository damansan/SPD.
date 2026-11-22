#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <d3d11.h>
#include "Model.h"
#include "Engine/Core/JsonFileCache.h"
#include "Engine/Data/AnimationEditData.h"

class ResourceManager
{
public:
    ResourceManager() = default;
    ~ResourceManager() = default;

    ResourceManager(const ResourceManager&) = delete;                // コピーコンストラクタ禁止
    ResourceManager& operator=(const ResourceManager&) = delete;     // コピー代入禁止
    ResourceManager(ResourceManager&&) noexcept = delete;            // ムーブコンストラクタ禁止
    ResourceManager& operator=(ResourceManager&&) noexcept = delete; // ムーブ代入禁止 

    // モデル取得(キャッシュ済みなら使い回す）
    std::shared_ptr<Model> GetModel(
        ID3D11Device* device,
        const std::string& filename
    );

    // キャッシュ削除
    void ReleaseModel(const std::string& filename);

    // 全キャッシュ削除
    void ReleaseAll();

private:
    // モデルキャッシュ
    std::unordered_map<std::string, std::shared_ptr<Model>> modelCache;
    // アニメーション編集
    JsonFileCache<AnimationEditData> editCache;
};