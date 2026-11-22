#include "Tool/Studio/StudioAnimator.h"
#include <imgui.h>


void StudioAnimator::DrawGUI(
    entt::registry& registry,
    StudioContext& context)
{
    if (!context.showAnimator) return;

    std::string panelName = "Animator##" + std::to_string(windowId);
    ImGui::Begin(panelName.c_str());

    ImGui::Text("Animator");
    ImGui::Separator();

    auto view = registry.view<SkinnedMeshComponent, AnimatorComponent>();

    if (view.begin() == view.end())
    {
        ImGui::TextColored({ 1, 0.5f, 0, 1 }, "No skinned model.");
        ImGui::End();
        return;
    }

    auto entity = *view.begin();
    auto& skinned = view.get<SkinnedMeshComponent>(entity);
    auto& animator = view.get<AnimatorComponent>(entity);

    if (!skinned.model)
    {
        ImGui::TextColored({ 1, 0.5f, 0, 1 }, "Model not loaded.");
        ImGui::End();
        return;
    }

    const auto& animations = skinned.model->GetAnimations();
    if (animations.empty())
    {
        ImGui::TextColored({ 1, 0.5f, 0, 1 }, "No animations.");
        ImGui::End();
        return;
    }

    ImGui::Text("Animations : %d", (int)animations.size());
    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));

    for (const auto& anim : animations)
    {
        bool isSelected = (animator.currentAnimation == anim.name);

        if (ImGui::Selectable(anim.name.c_str(), isSelected))
        {
            animator.currentAnimation = anim.name;
            animator.animationTime = 0.0f;
            animator.isPlaying = true;
            animator.isEnd = false;
        }

        if (isSelected)
            ImGui::SetItemDefaultFocus();
    }

    ImGui::PopStyleVar();

    ImGui::End();
}