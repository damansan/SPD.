#include "StudioTimeline.h"
#include <imgui.h>
#include <string>

void StudioTimeline::DrawGUI(entt::registry& registry, StudioContext& context)
{
    if (!context.showTimeline) return;

    std::string panelName = "Timeline##" + std::to_string(windowId);
    ImGui::Begin(panelName.c_str());

    auto view = registry.view<SkinnedMeshComponent, AnimatorComponent>();

    if (view.begin() == view.end())
    {
        ImGui::Text("No skinned model.");
        ImGui::End();
        return;
    }

    auto entity = *view.begin();
    auto& skinned = view.get<SkinnedMeshComponent>(entity);
    auto& animator = view.get<AnimatorComponent>(entity);

    if (!skinned.model)
    {
        ImGui::End();
        return;
    }

    // アニメーション長を取得
    float animLength = 0.0f;
    const auto& animations = skinned.model->GetAnimations();
    for (const auto& anim : animations)
    {
        if (anim.name == animator.currentAnimation)
        {
            animLength = anim.secondsLength;
            break;
        }
    }

    // 先頭に戻る
    if (ImGui::Button("<<"))
    {
        animator.animationTime = 0.0f;
        animator.isPlaying = false;
        animator.isEnd = false;
    }
    ImGui::SameLine();

    // 再生/停止
    if (animator.isPlaying)
    {
        if (ImGui::Button("■"))
        {
            animator.isPlaying = false;
        }
    }
    else
    {
        if (ImGui::Button("▶"))
        {
            if (animator.animationTime >= animLength)
                animator.animationTime = 0.0f;
            animator.isPlaying = true;
            animator.isEnd = false;
        }
    }
    ImGui::SameLine();

    // ループ
    ImGui::Checkbox("Loop", &animator.loop);
    ImGui::SameLine();

    // 現在のアニメーション名（緑色）
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
        animator.currentAnimation.c_str());

    // 時刻表示
    ImGui::Text("%.2f / %.2f s", animator.animationTime, animLength);
    ImGui::SameLine();

    // 再生時刻スライダー（全幅）
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##animTime",
        &animator.animationTime, 0.0f, animLength))
    {
        // スクラブ中は停止してポーズを確認
        animator.isPlaying = false;
        animator.isEnd = false;
        context.requestPoseUpdate = true;
    }

    ImGui::End();
}
