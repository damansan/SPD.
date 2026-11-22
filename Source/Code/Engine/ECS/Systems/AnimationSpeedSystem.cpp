#include "AnimationSpeedSystem.h"
#include "Engine/ECS/Components/SkinnedMeshComponent.h"
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/ECS/Components/AnimationSpeedComponent.h"


void AnimationSpeedSystem::Update(entt::registry& registry)
{
	auto view = registry.view<SkinnedMeshComponent, AnimatorComponent, AnimationSpeedComponent>();

	for (auto entity : view)
	{
		auto& skinned = view.get<SkinnedMeshComponent>(entity);
		auto& animator = view.get<AnimatorComponent>(entity);
		auto& animSpeed = view.get<AnimationSpeedComponent>(entity);

		// パスが未設定なら、このエンティティは速度カーブを使わない
		if (animSpeed.animationSpeedPath.empty())
		{
			animator.speedCurveMultiplier = 1.0f;
			continue;
		}

		if (!skinned.model)
		{
			continue;
		}

		AnimationSpeedData& data = dataCache.GetOrLoad(animSpeed.animationSpeedPath,
			[](AnimationSpeedData& data, const nlohmann::json& j) { data.FromJson(j); });

		const AnimationSpeedClipData* clip = nullptr;
		for (const auto& c : data.clips)
		{
			if (c.clipName == animator.currentAnimation) { clip = &c; break; }
		}

		// クリップの長さを取得する
		float clipLength = 0.0f;
		for (const auto& anim : skinned.model->GetAnimations())
		{
			if (anim.name == animator.currentAnimation)
			{
				clipLength = anim.secondsLength;
				break;
			}
		}

		if (clipLength <= 0.0001f)
		{
			animator.speedCurveMultiplier = 1.0f;
			continue;
		}

		float normalizedTime = animator.animationTime / clipLength;

		animator.speedCurveMultiplier = clip ? clip->EvaluateSpeed(normalizedTime) : 1.0f;
	}
}