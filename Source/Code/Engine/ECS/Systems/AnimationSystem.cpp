#include "AnimationSystem.h"
#include "tracy/Tracy.hpp"
#include "Engine/Resource/SkeletonUtils.h"
#include "Engine/ECS/Components/TransformComponent.h"
#include "Engine/ECS/Components/MovementComponent.h"

// 更新
void AnimationSystem::Update(
	entt::registry& registry,
	float elapsedTime)
{
	ZoneScoped;

	auto view = registry.view<AnimatorComponent, SkinnedMeshComponent>();
	view.each(
		[&](auto entity,
			AnimatorComponent& animator,
			SkinnedMeshComponent& skinned)
		{
			ZoneScopedN("AnimationSystem::Entity");

			// モデルが無効なら何もしない
			if (!skinned.model) return;

			// Inspector に実際に使っているルートノードを表示する
			const int rootMotionNodeIndex = skinned.model->GetRootNodeIndex();
			animator.activeRootMotionNode = (rootMotionNodeIndex >= 0) ? skinned.model->GetRootNodeName() : "";

			if (!animator.isPlaying)
			{
				// 止まっている間は、何も通過していない
				animator.stepFromTime = animator.animationTime;
				animator.stepToTime = animator.animationTime;
				animator.stepWrapped = false;
				return;
			}

			// 遷移した直後なら、前のフレームの最終ポーズを「混ぜる元」として保存する
			if (animator.blendSnapshotPending)
			{
				animator.blendSnapshotPending = false;

				if (skinned.localPositions.empty())
				{
					// まだ一度もポーズを作っていないので、混ぜる元が無い
					animator.isBlending = false;
				}
				else
				{
					animator.blendFromPositions = skinned.localPositions;
					animator.blendFromRotations = skinned.localRotations;
					animator.blendFromScales = skinned.localScales;
				}
			}

			const auto& animations = skinned.model->GetPlaybackAnimations(animator.usePreview);
			if (animations.empty()) return;

			// 再生するクリップを決める
			int animIndex = FindAnimationIndex(animations, animator.currentAnimation);
			if (animIndex < 0)
			{
				animIndex = 0;
				animator.currentAnimation = animations[0].name;
			}
			const auto& animation = animations[animIndex];

			// 時間を進める
			const ClipTimeStep step = AdvanceClipTime(animator, animation, elapsedTime);
			
			// 今回再生した区間を残す
			animator.stepFromTime = animator.clipStarted ? -1.0f : step.oldTime;
			animator.stepToTime = step.newTime;
			animator.stepWrapped = step.oldTime > step.newTime;
			animator.clipStarted = false;

			// ルートモーションの移動量
			animator.rootMotionDelta = (rootMotionNodeIndex >= 0) 
				? CalcRootMotionDelta(*skinned.model, animation, rootMotionNodeIndex, step.oldTime, step.newTime)
				: DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f };

			// ポーズを作る
			CalcBoneTransforms(
				*skinned.model, animation, animator.animationTime,
				skinned.localPositions, skinned.localRotations, skinned.localScales,
				rootMotionNodeIndex);

			// ルートモーション由来の速度
			auto* transform = registry.try_get<TransformComponent>(entity);
			auto* movement = registry.try_get<MovementComponent>(entity);

			RootMotionVelocity velocity;
			if (animator.useRootMotion && rootMotionNodeIndex >= 0 && elapsedTime > 0.0f && transform && movement)
			{
				velocity.value = ToWorldVelocity(animator.rootMotionDelta, *transform, elapsedTime, animator.useRootMotionY);
				velocity.valid = true;
				velocity.writeY = animator.useRootMotionY;
			}

			// クロスフェード
			if (animator.isBlending)
			{
				UpdateCrossfade(animator, skinned, elapsedTime);
			}

			// 速度を書く
			if (velocity.valid && movement)
			{
				movement->velocity.x = velocity.value.x;
				movement->velocity.z = velocity.value.z;
				if (velocity.writeY) movement->velocity.y = velocity.value.y;
			}

			// スキニング
			SkeletonUtils::CalcGlobalTransforms(
				skinned.model->GetNodes(), skinned.localPositions, skinned.localRotations, skinned.localScales,
				skinned.nodeGlobalTransforms);

			SkeletonUtils::CalcSkinningMatrices(
				*skinned.model, skinned.nodeGlobalTransforms, skinned.boneTransforms);
		});
}

// 時間を進めずに、指定した時刻のポーズだけ計算する
void AnimationSystem::EvaluatePose(entt::registry& registry)
{
	ZoneScoped;

	auto view = registry.view<AnimatorComponent, SkinnedMeshComponent>();
	view.each(
		[&](auto entity, AnimatorComponent& animator, SkinnedMeshComponent& skinned)
		{
			if (!skinned.model) return;

			// Inspector に実際に使っているルートノードを表示する
			const int rootMotionNodeIndex = skinned.model->GetRootNodeIndex();
			animator.activeRootMotionNode = (rootMotionNodeIndex >= 0) ? skinned.model->GetRootNodeName() : "";

			const auto& animations = skinned.model->GetPlaybackAnimations(animator.usePreview);
			if (animations.empty()) return;

			int animIndex = FindAnimationIndex(animations, animator.currentAnimation);
			if (animIndex < 0) return;

			const auto& animation = animations[animIndex];

			CalcBoneTransforms(
				*skinned.model, animation, animator.animationTime,
				skinned.localPositions, skinned.localRotations, skinned.localScales,
				rootMotionNodeIndex);

			SkeletonUtils::CalcGlobalTransforms(
				skinned.model->GetNodes(), skinned.localPositions, skinned.localRotations, skinned.localScales,
				skinned.nodeGlobalTransforms);

			SkeletonUtils::CalcSkinningMatrices(
				*skinned.model, skinned.nodeGlobalTransforms, skinned.boneTransforms);
		});
}

// 名前からクリップの番号を探す
int AnimationSystem::FindAnimationIndex(const std::vector<Model::Animation>& animations, const std::string& name)
{
	for (int i = 0; i < (int)animations.size(); i++)
	{
		if (animations[i].name == name) return i;
	}
	return -1;
}

// クリップの時間を1フレーム分進める
AnimationSystem::ClipTimeStep AnimationSystem::AdvanceClipTime(AnimatorComponent& animator, const Model::Animation& animation, float elapsedTime)
{
	const float length = animation.secondsLength;

	ClipTimeStep step;
	step.oldTime = animator.animationTime;

	animator.animationTime += elapsedTime * animator.playSpeed * animator.speedCurveMultiplier;

	if (animator.loop)
	{
		// ループ：終わりを越えたら、越えた分だけ頭から
		if (animator.animationTime > length)
		{
			animator.animationTime -= length;
			animator.isEnd = false;
		}
	}
	else
	{
		// ループしない：終わりで止める
		if (animator.animationTime >= length)
		{
			animator.animationTime = length;
			animator.isPlaying = false;
			animator.isEnd = true;
		}
	}

	step.newTime = animator.animationTime;
	return step;
}

// ルートモーションのローカルの移動量を、ワールドの速度に変換する
DirectX::XMFLOAT3 AnimationSystem::ToWorldVelocity(const DirectX::XMFLOAT3& localDelta, const TransformComponent& transform, float elapsedTime, bool useY)
{
	DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(
		transform.rotation.x, transform.rotation.y, transform.rotation.z);

	DirectX::XMFLOAT3 worldDelta;
	DirectX::XMStoreFloat3(&worldDelta,
		DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&localDelta), rotationMatrix));

	return {
		worldDelta.x / elapsedTime,
		useY ? worldDelta.y / elapsedTime : 0.0f,
		worldDelta.z / elapsedTime };
}

// クロスフェード：保存したポーズから、今のポーズへ混ぜる
void AnimationSystem::UpdateCrossfade(AnimatorComponent& animator, SkinnedMeshComponent& skinned, float elapsedTime)
{
	// 混ぜる元のポーズと、今のポーズのボーン数が違う場合は、混ぜるのをやめる
	if (animator.blendFromPositions.size() != skinned.localPositions.size())
	{
		animator.isBlending = false;
		return;
	}

	// 進み具合を進める
	animator.blendRate += (animator.blendTime > 0.0f) ? (elapsedTime / animator.blendTime) : 1.0f;

	// 進み具合は一定の速さで進め、混ぜる割合にだけ緩急を付ける
	const float linear = std::clamp(animator.blendRate, 0.0f, 1.0f);
	const float rate = linear * linear * (3.0f - 2.0f * linear);
	
	// 混ぜる元 → 今のポーズ
	BlendPoses(
		animator.blendFromPositions, animator.blendFromRotations, animator.blendFromScales,
		skinned.localPositions, skinned.localRotations, skinned.localScales,
		rate,
		skinned.localPositions, skinned.localRotations, skinned.localScales);

	if (animator.blendRate >= 1.0f)
	{
		animator.isBlending = false;
	}
}

// キーフレーム補間（位置）
DirectX::XMFLOAT3 AnimationSystem::InterpolatePosition(
	const std::vector<Model::VectorKeyframe>& keyframes,
	float time)
{
	if (keyframes.size() == 1)
	{
		return keyframes[0].value;
	}

	// 前後のキーフレームを探す
	for (int i = 0; i < (int)keyframes.size() - 1; i++)
	{
		if (time <= keyframes[i + 1].seconds)
		{
			float t = (time - keyframes[i].seconds) / (keyframes[i + 1].seconds - keyframes[i].seconds);

			DirectX::XMVECTOR a = DirectX::XMLoadFloat3(
				&keyframes[i].value);
			DirectX::XMVECTOR b = DirectX::XMLoadFloat3(
				&keyframes[i + 1].value);

			DirectX::XMFLOAT3 result;
			DirectX::XMStoreFloat3(
				&result,
				DirectX::XMVectorLerp(a, b, t));
			return result;
		}
	}
	// 末尾を返す
	return keyframes.back().value;
}

// キーフレーム補間（回転）
DirectX::XMFLOAT4 AnimationSystem::InterpolateRotation(
	const std::vector<Model::QuaternionKeyframe>& keyframes,
	float time)
{
	if (keyframes.size() == 1)
	{
		return keyframes[0].value;
	}

	for (int i = 0; i < (int)keyframes.size() - 1; i++)
	{
		if (time <= keyframes[i + 1].seconds)
		{
			float t = (time - keyframes[i].seconds) / (keyframes[i + 1].seconds - keyframes[i].seconds);

			DirectX::XMVECTOR a = DirectX::XMLoadFloat4(
				&keyframes[i].value);
			DirectX::XMVECTOR b = DirectX::XMLoadFloat4(
				&keyframes[i + 1].value);

			DirectX::XMFLOAT4 result;
			DirectX::XMStoreFloat4(
				&result,
				DirectX::XMQuaternionSlerp(a, b, t));
			return result;
		}
	}
	return keyframes.back().value;
}

// キーフレーム補間（スケール）
DirectX::XMFLOAT3 AnimationSystem::InterpolateScale(
	const std::vector<Model::VectorKeyframe>& keyframes,
	float time)
{
	if (keyframes.size() == 1)
	{
		return keyframes[0].value;
	}

	for (int i = 0; i < (int)keyframes.size() - 1; i++)
	{
		if (time <= keyframes[i + 1].seconds)
		{
			float t = (time - keyframes[i].seconds) / (keyframes[i + 1].seconds - keyframes[i].seconds);

			DirectX::XMVECTOR a = DirectX::XMLoadFloat3(
				&keyframes[i].value);
			DirectX::XMVECTOR b = DirectX::XMLoadFloat3(
				&keyframes[i + 1].value);

			DirectX::XMFLOAT3 result;
			DirectX::XMStoreFloat3(
				&result,
				DirectX::XMVectorLerp(a, b, t));
			return result;
		}
	}
	return keyframes.back().value;
}

// ノードのグローバル行列を再帰計算してボーン行列に書き込む
void AnimationSystem::CalcBoneTransforms(
	Model& model,
	const Model::Animation& animation,
	float time,
	std::vector<DirectX::XMFLOAT3>& outPositions,
	std::vector<DirectX::XMFLOAT4>& outRotations,
	std::vector<DirectX::XMFLOAT3>& outScales,
	int rootMotionNodeIndex) 
{
	auto& nodes = model.GetNodes();

	outPositions.resize(nodes.size());
	outRotations.resize(nodes.size());
	outScales.resize(nodes.size());

	for (int i = 0; i < (int)nodes.size(); i++)
	{
		outPositions[i] = nodes[i].position;
		outRotations[i] = nodes[i].rotation;
		outScales[i] = nodes[i].scale;

		// キーフレームがあれば補間して上書き
		if (i < (int)animation.nodeAnims.size())
		{
			const auto& nodeAnim = animation.nodeAnims[i];

			if (!nodeAnim.positionKeyframes.empty())
			{
				outPositions[i] = InterpolatePosition(nodeAnim.positionKeyframes, time);
			}
			if (!nodeAnim.rotationKeyframes.empty())
			{
				outRotations[i] = InterpolateRotation(nodeAnim.rotationKeyframes, time);
			}
			if (!nodeAnim.scaleKeyframes.empty())
			{
				outScales[i] = InterpolateScale(nodeAnim.scaleKeyframes, time);
			}
		}

		// ルートモーション対象ノードはバインドポーズの位置に固定する
		if (i == rootMotionNodeIndex)
		{
			outPositions[i] = nodes[i].position;
		}
	}
}

// 2つのポーズをrateで補間する
void AnimationSystem::BlendPoses(
	const std::vector<DirectX::XMFLOAT3>& posA,
	const std::vector<DirectX::XMFLOAT4>& rotA,
	const std::vector<DirectX::XMFLOAT3>& scaleA,
	const std::vector<DirectX::XMFLOAT3>& posB,
	const std::vector<DirectX::XMFLOAT4>& rotB,
	const std::vector<DirectX::XMFLOAT3>& scaleB,
	float rate,
	std::vector<DirectX::XMFLOAT3>& outPositions,
	std::vector<DirectX::XMFLOAT4>& outRotations,
	std::vector<DirectX::XMFLOAT3>& outScales)
{
	size_t count = posA.size();

	std::vector<DirectX::XMFLOAT3> tempPos(count);
	std::vector<DirectX::XMFLOAT4> tempRot(count);
	std::vector<DirectX::XMFLOAT3> tempScale(count);

	for (size_t i = 0; i < count; i++)
	{
		DirectX::XMStoreFloat3(&tempPos[i],
			DirectX::XMVectorLerp(DirectX::XMLoadFloat3(&posA[i]), DirectX::XMLoadFloat3(&posB[i]), rate));

		DirectX::XMStoreFloat4(&tempRot[i],
			DirectX::XMQuaternionSlerp(DirectX::XMLoadFloat4(&rotA[i]), DirectX::XMLoadFloat4(&rotB[i]), rate));

		DirectX::XMStoreFloat3(&tempScale[i],
			DirectX::XMVectorLerp(DirectX::XMLoadFloat3(&scaleA[i]), DirectX::XMLoadFloat3(&scaleB[i]), rate));
	}

	outPositions = std::move(tempPos);
	outRotations = std::move(tempRot);
	outScales = std::move(tempScale);
}

// 指定時刻のローカル位置を求める
DirectX::XMFLOAT3 AnimationSystem::CalcNodeLocalPosition(
	const Model::Animation& animation,
	int nodeIndex,
	const Model::Node& node,
	float time)
{
	// アニメーションが無ければバインドポーズの位置を返す
	if (nodeIndex >= (int)animation.nodeAnims.size())
	{
		return node.position;
	}

	const auto& nodeAnim = animation.nodeAnims[nodeIndex];
	if (nodeAnim.positionKeyframes.empty())
	{
		return node.position;
	}

	return InterpolatePosition(nodeAnim.positionKeyframes, time);
}

// ルートモーションの移動量を計算する
DirectX::XMFLOAT3 AnimationSystem::CalcRootMotionDelta(
	const Model& model,
	const Model::Animation& animation,
	int rootMotionNodeIndex,
	float oldTime,
	float newTime)
{
	const auto& node = model.GetNodes()[rootMotionNodeIndex];

	// ポーズ取得用のラムダ
	auto getPos = [&](float t)
		{
			return CalcNodeLocalPosition(animation, rootMotionNodeIndex, node, t);
		};

	DirectX::XMFLOAT3 delta{ 0.0f, 0.0f, 0.0f };

	if (oldTime > newTime)
	{
		// ループした：進む前からクリップの最後まで ＋ クリップの頭から進んだ後まで
		DirectX::XMFLOAT3 beginPos = getPos(0.0f);
		DirectX::XMFLOAT3 oldPos = getPos(oldTime);
		DirectX::XMFLOAT3 endPos = getPos(animation.secondsLength);
		DirectX::XMFLOAT3 newPos = getPos(newTime);

		delta.x = (endPos.x - oldPos.x) + (newPos.x - beginPos.x);
		delta.y = (endPos.y - oldPos.y) + (newPos.y - beginPos.y);
		delta.z = (endPos.z - oldPos.z) + (newPos.z - beginPos.z);
	}
	else
	{
		DirectX::XMFLOAT3 oldPos = getPos(oldTime);
		DirectX::XMFLOAT3 newPos = getPos(newTime);

		delta.x = newPos.x - oldPos.x;
		delta.y = newPos.y - oldPos.y;
		delta.z = newPos.z - oldPos.z;
	}

	return delta;
}
