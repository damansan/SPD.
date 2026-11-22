#include "MeshLoaderSystem.h"

void MeshLoaderSystem::Update(
    entt::registry& registry,
    ID3D11Device* device,
    ResourceManager& resourceManager)
{

    // 自分が作った派生を外す
    auto removeDerived = [&](entt::entity entity)
        {
            registry.remove<MeshLoadedTag, SkinnedMeshComponent, StaticMeshComponent>(entity);
        };

    // Mesh が外されていたら、派生も外す
    auto orphanView = registry.view<MeshLoadedTag>(entt::exclude<MeshComponent>);
    const std::vector<entt::entity> orphans(orphanView.begin(), orphanView.end());
    for (auto entity : orphans)
    {
        removeDerived(entity);
    }

    // パスが変わっていたら、派生を外して再ロードさせる
    auto loadedView = registry.view<MeshComponent, MeshLoadedTag>();
    for (auto entity : loadedView)
    {
        auto& mesh = loadedView.get<MeshComponent>(entity);
        if (mesh.modelPath != mesh.previousPath)
        {
            removeDerived(entity);
        }
    }

    // MeshComponentを持つがMeshLoadedTagのないEntityを取得
    auto view = registry.view<MeshComponent>(entt::exclude<MeshLoadedTag>);

    for (auto entity : view)
    {
        auto& mesh = view.get<MeshComponent>(entity);

        // モデルパスが空ならスキップ
        if (mesh.modelPath.empty()) 
        {
            continue;
        }

        // モデル読み込み（キャッシュ利用）
        auto model = resourceManager.GetModel(device, mesh.modelPath);
        if (!model) continue;

        // スキニング有無で自動判定
        if (model->HasSkin())
        {
            // スキニングあり  SkinnedMeshComponent 付与
            auto& skinned = registry.emplace<SkinnedMeshComponent>(entity);
            skinned.model = model;
            skinned.modelPath = mesh.modelPath;

            // ボーン行列初期化（メッシュごと）
            const auto& meshes = model->GetMeshes();
            skinned.boneTransforms.resize(meshes.size());
            for (int m = 0; m < (int)meshes.size(); m++)
            {
                skinned.boneTransforms[m].resize(
                    meshes[m].bones.size(),
                    { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 }
                );
            }
        }
        else
        {
            // スキニングなし → StaticMeshComponent付与
            auto& staticMesh = registry.emplace<StaticMeshComponent>(entity);
            staticMesh.model = model;
        }

        // ロード完了時に previousPath を更新
        mesh.previousPath = mesh.modelPath;

        // ロード済みフラグを付与
        registry.emplace<MeshLoadedTag>(entity);
    }
}