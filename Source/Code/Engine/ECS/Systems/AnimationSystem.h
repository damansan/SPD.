#pragma once
#include <d3d11.h>
#include <algorithm>
#include "entt.hpp"
#include "Engine/Resource/Model.h" 
#include "Engine/ECS/Components/AnimatorComponent.h"
#include "Engine/ECS/Components/SkinnedMeshComponent.h"

struct TransformComponent;

class AnimationSystem
{
public:
	// 更新（時間を進めてボーン行列を計算）
	void Update(
		entt::registry& registry,
		float elapsedTime);

	// 時間を進めずに、指定した時刻のポーズだけ計算する
	void EvaluatePose(entt::registry& registry);

private:
	// 1フレーム分、クリップの時間を進めた結果
	struct ClipTimeStep
	{
		float oldTime = 0.0f; // 進める前の時間
		float newTime = 0.0f; // 進めた後の時間
	};

	// ルートモーション由来の速度
	struct RootMotionVelocity
	{
		DirectX::XMFLOAT3 value{ 0.0f, 0.0f, 0.0f };
		bool valid = false;  // XZ 書き込むか
		bool writeY = false; // Y も書き込むか
	};

	// 名前からクリップの番号を探す
	static int FindAnimationIndex(const std::vector<Model::Animation>& animations, const std::string& name);

	// クリップの時間を1フレーム分進める
	static ClipTimeStep AdvanceClipTime(AnimatorComponent& animator, const Model::Animation& animation, float elapsedTime);

	// ルートモーションのローカルの移動量を、ワールドの速度に変換する
	static DirectX::XMFLOAT3 ToWorldVelocity(const DirectX::XMFLOAT3& localDelta, const TransformComponent& transform, float elapsedTime, bool useY);

	// クロスフェード：保存したポーズから、今のポーズへ混ぜる
	static void UpdateCrossfade(AnimatorComponent& animator, SkinnedMeshComponent& skinned, float elapsedTime);


	// キーフレーム補間（位置）
	static DirectX::XMFLOAT3 InterpolatePosition(
		const std::vector<Model::VectorKeyframe>& keyframes,
		float time);

	// キーフレーム補間（回転）
	static DirectX::XMFLOAT4 InterpolateRotation(
		const std::vector<Model::QuaternionKeyframe>& keyframes,
		float time);

	// キーフレーム補間（スケール）
	static DirectX::XMFLOAT3 InterpolateScale(
		const std::vector<Model::VectorKeyframe>& keyframes,
		float time);

	// ノードのグローバル行列を再帰計算してボーン行列に書き込む
	static void CalcBoneTransforms(
		Model& model,
		const Model::Animation& animation,
		float time,
		std::vector<DirectX::XMFLOAT3>& outPositions,
		std::vector<DirectX::XMFLOAT4>& outRotations,
		std::vector<DirectX::XMFLOAT3>& outScales,
		int rootMotionNodeIndex = -1);

	// 2つのポーズをrateで補間する
	static void BlendPoses(
		const std::vector<DirectX::XMFLOAT3>& posA, const std::vector<DirectX::XMFLOAT4>& rotA, const std::vector<DirectX::XMFLOAT3>& scaleA,
		const std::vector<DirectX::XMFLOAT3>& posB, const std::vector<DirectX::XMFLOAT4>& rotB, const std::vector<DirectX::XMFLOAT3>& scaleB,
		float rate,
		std::vector<DirectX::XMFLOAT3>& outPositions,
		std::vector<DirectX::XMFLOAT4>& outRotations,
		std::vector<DirectX::XMFLOAT3>& outScales);

	// 指定時刻のローカル位置を求める
	static DirectX::XMFLOAT3 CalcNodeLocalPosition(
		const Model::Animation& animation,
		int nodeIndex,
		const Model::Node& node,
		float time);

	// ルートモーションの移動量を計算する
	static DirectX::XMFLOAT3 CalcRootMotionDelta(
		const Model& model,
		const Model::Animation& animation,
		int rootMotionNodeIndex,
		float oldTime,
		float newTime);
};