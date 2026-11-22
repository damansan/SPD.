#include "SequencerSystem.h"
#include "Engine/ECS/Components/SkinnedMeshComponent.h"
#include "Engine/ECS/Components/AnimatorComponent.h"

// 1体のエンティティについて、フラグを更新する
void SequencerSystem::UpdateFlags(SequencerComponent& seq, const SequencerClipData* clip, const PlayedSpan& span)
{
	// フラグを一旦全部falseに戻す
	seq.animExit = false;

	if (!clip) return;

	auto setFlag = [&](SequencerFlag flag, bool value)
		{
			switch (flag)
			{
			case SequencerFlag::AnimExit: seq.animExit = seq.animExit || value; break;
			default: break;
			}
		};

	// Range：今の時刻が範囲内ならtrue
	for (const auto& r : clip->ranges)
	{
		setFlag(r.flag, span.current >= r.start && span.current <= r.end);
	}

	// Point：今回のフレームで再生した区間に入っていればtrue
	for (const auto& p : clip->points)
	{
		bool passed = false;
		if (!span.wrapped)
		{
			passed = (span.from < p.time && p.time <= span.to);
		}
		else
		{
			// ループしたフレーム：進む前からクリップの最後 と、クリップの頭から進んだ後
			passed = (span.from < p.time && p.time <= span.clipEnd)
				|| (span.clipStart <= p.time && p.time <= span.to);
		}
		setFlag(p.flag, passed);
	}
}

void SequencerSystem::Update(entt::registry& registry)
{
	auto view = registry.view<SkinnedMeshComponent, AnimatorComponent, SequencerComponent>();

	for (auto entity : view)
	{
		auto& skinned = view.get<SkinnedMeshComponent>(entity);
		auto& animator = view.get<AnimatorComponent>(entity);
		auto& seq = view.get<SequencerComponent>(entity);

		if (!skinned.model || skinned.modelPath.empty())
		{
			continue;
		}

		// モデルごとのファイルを読む
		const SequencerData& data = dataCache.GetOrLoad(SequencerData::GetFilePath(skinned.modelPath),
			[](SequencerData& data, const nlohmann::json& j) { data.FromJson(j); }, JsonMissingLog::Silent);

		const SequencerClipData* clip = data.FindClip(animator.currentAnimation);

		// 焼き込み後のクリップを探す
		const Model::Animation* animation = skinned.model->FindAnimation(animator.currentAnimation);

		if (!animation)
		{
			continue;
		}

		// 今回のフレームで再生した区間を、生データの時刻に直す
		PlayedSpan span;
		span.current = animation->ToRawTime(animator.animationTime);
		span.clipStart = animation->ToRawTime(0.0f);
		span.clipEnd = animation->ToRawTime(animation->secondsLength);
		span.to = animation->ToRawTime(animator.stepToTime);
		span.wrapped = animator.stepWrapped;

		// 始まった直後は、頭の時刻ちょうども含むように、頭より少し前にする
		span.from = (animator.stepFromTime < 0.0f)
			? span.clipStart - 0.0001f
			: animation->ToRawTime(animator.stepFromTime);

		UpdateFlags(seq, clip, span);
	}
}