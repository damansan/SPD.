#include "StudioModelViewer.h"

#include <windows.h>
#include <commdlg.h>
#include <imgui.h>

#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/ECS/Components/MaterialComponent.h"
#include "Engine/ECS/Components/AnimationSpeedComponent.h"

// 初期化
void StudioModelViewer::Initialize(
    ID3D11Device* device,
    ResourceManager* resourceManager)
{
    this->device = device;
    this->resourceManager = resourceManager;
}

// 更新
void StudioModelViewer::Update(float elapsedTime, StudioContext& context)
{
}

// ImGui
void StudioModelViewer::DrawGUI(
    entt::registry& registry, 
    StudioContext& context)
{
    //ImGui::Begin("Model Viewer");

    //// ロードボタン
    //if (ImGui::Button("Load Model"))
    //{
    //    std::string path = OpenFileDialog();
    //    if (!path.empty())
    //    {
    //        LoadModel(registry, path);
    //    }
    //}

    //// 現在のモデルパスを表示
    //if (!currentModelPath.empty())
    //{
    //    ImGui::SameLine();
    //    ImGui::TextUnformatted(currentModelPath.c_str());
    //}

    //ImGui::End();
}

// ファイルダイアログ
std::string StudioModelViewer::OpenFileDialog()
{
    char path[MAX_PATH] = {};

    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = "GLTF Files\0*.gltf;*.glb\0All Files\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Open Model";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn))
    {
        // 絶対パスをカレントディレクトリからの相対パスに変換
        char currentDir[MAX_PATH] = {};
        GetCurrentDirectoryA(MAX_PATH, currentDir);

        std::string absPath = path;
        std::string basePath = std::string(currentDir) + "\\";
        if (absPath.find(basePath) == 0)
            absPath = absPath.substr(basePath.size());

        // バックスラッシュをスラッシュに統一
        std::replace(absPath.begin(), absPath.end(), '\\', '/');

        return absPath;
    }
    return {};
}

// モデルロード
void StudioModelViewer::LoadModel(
    entt::registry& registry,
    const std::string& path)
{
    // 既存のEntityを削除
    if (modelEntity != entt::null)
    {
        registry.destroy(modelEntity);
        modelEntity = entt::null;
    }

    // モデルをロード
    auto model = resourceManager->GetModel(device, path);
    if (!model) return;

    // Entityを作成
    modelEntity = registry.create();

    // TransformComponent追加
    registry.emplace<TransformComponent>(modelEntity);

    // MeshComponent追加
    auto& mesh = registry.emplace<MeshComponent>(modelEntity);
    mesh.modelPath = path;
    mesh.previousPath = path;
    // MaterialComponent追加
    registry.emplace<MaterialComponent>(modelEntity);

    // スキニングの有無で分岐
    if (model->HasSkin())
    {
        auto& skinned = registry.emplace<SkinnedMeshComponent>(modelEntity);
        skinned.model = model;

        const auto& meshes = model->GetMeshes();
        skinned.boneTransforms.resize(meshes.size());
        for (int i = 0; i < (int)meshes.size(); i++)
        {
            skinned.boneTransforms[i].resize(
                meshes[i].bones.size(),
                { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 }
            );
        }
    }
    else
    {
        auto& staticMesh = registry.emplace<StaticMeshComponent>(modelEntity);
        staticMesh.model = model;
    }

    // スキニングありの場合はAnimatorComponentを付与
    if (model->HasSkin() && !model->GetAnimations().empty())
    {
        auto& animator = registry.emplace<AnimatorComponent>(modelEntity);
        animator.currentAnimation = model->GetAnimations()[0].name;
        animator.isPlaying = false;
        animator.loop = false;

        registry.emplace<AnimationSpeedComponent>(modelEntity);
    }

    // ロード済みフラグ
    registry.emplace<MeshLoadedTag>(modelEntity);

    currentModelPath = path;
}
