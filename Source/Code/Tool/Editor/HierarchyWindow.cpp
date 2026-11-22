#include "HierarchyWindow.h"
#include "imgui_internal.h"
#include <imgui.h>
#include <filesystem>
#include <fstream>

#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Components/MeshComponent.h"
#include "Engine/ECS/Components/MaterialComponent.h"
#include "Engine/ECS/Components/CategoryComponent.h"
#include "Engine/ECS/Components/PrefabInstanceComponent.h"

#include "Engine/Resource/EntitySerializer.h"

#include "tracy/Tracy.hpp"

void HierarchyWindow::Draw(entt::registry& registry, EditorContext& context)
{
    ZoneScoped;

    ImGui::Begin("Hierarchy");

    // 空の Entity を追加するボタン
    if (ImGui::Button("+ Add Entity"))
    {
        entt::entity e = registry.create();

        registry.emplace<CategoryComponent>(e);
        registry.emplace<TransformComponent>(e);

        // 名前を自動設定
        context.SetName(e, "Entity");

        // 作成した Entity を選択状態に
        context.Select(e);
    }
    // 選択中のときだけ Delete ボタンを隣に表示
    if (context.selectedEntity != entt::null)
    {
        ImGui::SameLine();
        if (ImGui::Button("Delete"))
        {
            registry.destroy(context.selectedEntity);
            context.Deselect();
        }
    }

    ImGui::Separator();
    // セパレータの下の座標を記録
    ImVec2 dropAreaMin = ImGui::GetCursorScreenPos();

    // 全 Entity を列挙(画面に映っている範囲だけ処理する)
    auto view = registry.view<entt::entity>();

    // entt::view はランダムアクセスできないので、一旦配列に集める
    // (これ自体は軽い処理:ポインタ相当のentity値をコピーするだけ)
    std::vector<entt::entity> entities(view.begin(), view.end());

    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(entities.size()));
    while (clipper.Step())
    {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
            entt::entity e = entities[i];

            std::string name = context.GetName(e);

            // プレハブのインスタンスは、名前の色を変えて見分けられるようにする
            bool isPrefabInstance = registry.any_of<PrefabInstanceComponent>(e);
            if (isPrefabInstance)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f)); // 薄い青
            }

            // 選択状態をハイライト
            bool isSelected = context.IsSelected(e);
            if (ImGui::Selectable(
                (name + "##" + std::to_string((uint32_t)e)).c_str(),
                isSelected))
            {
                context.Select(e);
            }

            if (isPrefabInstance)
            {
                ImGui::PopStyleColor();
            }

            // Entity をドラッグできるようにする
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                // Entity ID をペイロードとして渡す
                ImGui::SetDragDropPayload(
                    "ENTITY",
                    &e,
                    sizeof(entt::entity)
                );
                // ドラッグ中のプレビュー
                ImGui::Text(name.c_str());
                ImGui::EndDragDropSource();
            }
        }
    }

    // セパレータ以下ウィンドウ末尾までをドロップターゲットに
    ImVec2 dropAreaMax = ImGui::GetWindowPos();
    dropAreaMax.x += ImGui::GetWindowSize().x;
    dropAreaMax.y += ImGui::GetWindowSize().y;

    ImRect dropRect(dropAreaMin, dropAreaMax);

	// モデルのドロップ受け取り
    if (ImGui::BeginDragDropTargetCustom(dropRect, ImGui::GetCurrentWindow()->ID))
    {
        if (const ImGuiPayload* payload =
            ImGui::AcceptDragDropPayload("MODEL_PATH"))
        {                
            // パスを受け取る
            std::string modelPath(static_cast<const char*>(payload->Data));

            // Entity 生成
            entt::entity e = registry.create();

            registry.emplace<CategoryComponent>(e);

            auto& t = registry.emplace<TransformComponent>(e);
            t.position = { 0.0f, 0.0f, 0.0f };
            t.scale = { 1.0f, 1.0f, 1.0f };

            registry.emplace<MeshComponent>(e, modelPath);
            registry.emplace<MaterialComponent>(e);

            // ファイル名から Entity 名を設定
            std::filesystem::path p(modelPath);
            context.SetName(e, p.stem().string());
            context.Select(e);
        }

        // プレハブのドロップ受け取り
        if (const ImGuiPayload* payload =
            ImGui::AcceptDragDropPayload("PREFAB_PATH"))
        {
            // プレハブのパスを受け取る
            std::string prefabPath(static_cast<const char*>(payload->Data));

            // プレハブからEntityを生成する（読み込みに失敗した場合は、何も置かれない）
            entt::entity e = EntitySerializer::InstantiatePrefab(registry, prefabPath);
            if (e != entt::null)
            {
                // ファイル名（.prefab.jsonを除いた部分）から Entity 名を設定
                std::filesystem::path p(prefabPath);
                std::string stem = p.stem().string(); // "xxx.prefab"
                std::filesystem::path stemPath(stem);
                context.SetName(e, stemPath.stem().string()); // "xxx"

                context.Select(e);
            }
        }

        ImGui::EndDragDropTarget();
    }
    ImGui::End();
}