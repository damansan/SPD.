#include "InspectorWindow.h"
#include <imgui.h>
#include "imgui_internal.h"
#include "Engine/Resource/ComponentRegistry.h"
#include "Tool/Editor/ComponentInspectorRegistry.h"
#include "Tool/Editor/MetaRegistry.h"
#include "tracy/Tracy.hpp"

// 描画
void InspectorWindow::Draw(entt::registry& registry, EditorContext& context)
{
    ZoneScoped;

    ImGui::Begin("Inspector");

    // 何も選択されていなければ何も表示しない
    if (context.selectedEntity == entt::null)
    {
        ImGui::Text("No entity selected.");
        ImGui::End();
        return;
    }

    // 名前編集
    DrawNameField(registry, context);

    ImGui::Separator();

    // コンポーネント一覧
    DrawComponents(registry, context);

    ImGui::Separator();

    // コンポーネント追加ボタン
    DrawAddComponent(registry, context);

    ImGui::End();
}

// 名前編集UI
void InspectorWindow::DrawNameField(entt::registry& registry, EditorContext& context)
{
    entt::entity e = context.selectedEntity;
    std::string name = context.GetName(e);

    char buf[256] = {};
    strncpy_s(buf, name.c_str(), sizeof(buf) - 1);

    if (ImGui::InputText("Name", buf, sizeof(buf)))
    {
        context.SetName(e, buf);
    }
}

// コンポーネント一覧UI
void InspectorWindow::DrawComponents(entt::registry& registry, EditorContext& context)
{
    entt::entity e = context.selectedEntity;

    for (const auto& name : ComponentRegistry::GetTypeNames())
    {
        const ComponentInfo* info = ComponentRegistry::Get(name);
        if (!info) continue;
        // このEntityがコンポーネントを持っているか
        if (!info->has(registry, e)) continue;

        ImGui::PushID(name.c_str());

        // 削除ボタン
        if (ImGui::Button("x"))
        {
            info->remove(registry, e);
            ImGui::PopID();
            // 削除後はイテレーションを中断
            break;
        }
        ImGui::SameLine();


        // コンポーネントのImGuiを描画
        const auto* drawImGui = ComponentInspectorRegistry::Get(name);
        if (drawImGui)
        {
            // 独自のUIが登録されていればそれを使う
            (*drawImGui)(registry, e);
        }
        else if (info->getMeta)
        {
            entt::meta_any firstInstance = info->getMeta(registry, e);
            if (firstInstance)
            {
                entt::meta_type metaType = firstInstance.type();
                TypeInfo* typeInfo = metaType.custom();
                std::string label = typeInfo ? typeInfo->category : name;

                if (ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                {
                    // 別のID空間で囲む
                    ImGui::PushID("fields");
                    MetaRegistry::DrawMetaFields(metaType, info->getMeta(registry, e));
                    ImGui::PopID();
                }
            }
        }

        ImGui::PopID();
    }
}

// コンポーネント追加UI
void InspectorWindow::DrawAddComponent(entt::registry& registry, EditorContext& context)
{
    entt::entity e = context.selectedEntity;

    if (ImGui::Button("+ Add Component"))
    {
        ImGui::OpenPopup("AddComponentPopup");
    }

    if (ImGui::BeginPopup("AddComponentPopup"))
    {
        ImGui::Text("Select Component");
        ImGui::Separator();

        for (const auto& name : ComponentRegistry::GetTypeNames())
        {
            const ComponentInfo* info = ComponentRegistry::Get(name);
            if (!info) continue;

            // すでに持っているコンポーネントはグレーアウト
            if (info->has(registry, e))
            {
                ImGui::TextDisabled("%s", info->displayName.c_str());
            }
            else
            {
                if (ImGui::Selectable(info->displayName.c_str()))
                {
                    info->add(registry, e);
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        ImGui::EndPopup();
    }
}
