#include "IKSystem.h"
#include "Engine/ECS/Systems/TwoBoneIKSolver.h"
#include "Engine/Resource/SkeletonUtils.h"
#include "Engine/ECS/Systems/PhysicsSystem.h"
#include "Engine/Core/PhysicsLayer.h"

namespace
{
	// ワールド回転を、親のワールド回転を基準にしたローカル回転に変換する
	DirectX::XMVECTOR WorldToLocalRotation(DirectX::FXMVECTOR worldRotation, DirectX::FXMVECTOR parentWorldRotation)
	{
		DirectX::XMVECTOR parentInverse = DirectX::XMQuaternionInverse(parentWorldRotation);
		// worldRotation = localRotation * parentWorldRotation なので
		// localRotation = worldRotation * (parentWorldRotation)^-1
		return DirectX::XMQuaternionMultiply(worldRotation, parentInverse);
	}

	// XMFLOAT4X4(ワールド行列)から、位置と回転(クォータニオン)を取り出す
	void DecomposePosRot(const DirectX::XMFLOAT4X4& world, DirectX::XMFLOAT3& outPos, DirectX::XMFLOAT4& outRot)
	{
		DirectX::XMMATRIX m = DirectX::XMLoadFloat4x4(&world);

		DirectX::XMVECTOR scale, rotQuat, trans;
		DirectX::XMMatrixDecompose(&scale, &rotQuat, &trans, m);

		DirectX::XMStoreFloat3(&outPos, trans);
		DirectX::XMStoreFloat4(&outRot, rotQuat);
	}
}

void IKSystem::Update(entt::registry& registry, physx::PxScene* pxScene)
{
	auto view = registry.view<IKComponent, SkinnedMeshComponent, TransformComponent>();
	view.each(
		[&](auto entity, IKComponent& ik, SkinnedMeshComponent& skinned, TransformComponent& transform)
		{
			if (!ik.enabled) return;
			if (!skinned.model) return;
			if (skinned.nodeGlobalTransforms.empty()) return;

			DirectX::XMFLOAT4X4 entityWorldMatrix;
			DirectX::XMStoreFloat4x4(&entityWorldMatrix, transform.GetWorldMatrix());

			for (auto& footSettings : ik.feet)
			{
				SolveFootIK(footSettings, skinned, pxScene, entityWorldMatrix);
			}
		});
}

// 1本の足のFootIK処理
void IKSystem::SolveFootIK(FootIKSettings& footSettings, SkinnedMeshComponent& skinned, physx::PxScene* pxScene, const DirectX::XMFLOAT4X4& entityWorldMatrix)
{
	// ボーンインデックス
	const int rootIndex = footSettings.rootBoneIndex;
	const int midIndex = footSettings.midBoneIndex;
	const int endIndex = footSettings.endBoneIndex;

	auto& globals = skinned.nodeGlobalTransforms;
	if (rootIndex < 0 || midIndex < 0 || endIndex < 0 ||
		rootIndex >= (int)globals.size() ||
		midIndex >= (int)globals.size() ||
		endIndex >= (int)globals.size())
	{
		return;
	}

	// 現在のワールド位置・回転を取り出す
	DirectX::XMFLOAT3 rootPos, midPos, endPos;
	DirectX::XMFLOAT4 rootRot, midRot, endRot;
	DecomposePosRot(globals[rootIndex], rootPos, rootRot);
	DecomposePosRot(globals[midIndex], midPos, midRot);
	DecomposePosRot(globals[endIndex], endPos, endRot);

	// 初回だけ、現在の足首位置をtargetPosの初期値として使う
	if (!footSettings.hasInitializedTarget)
	{
		footSettings.targetPos = endPos;
		footSettings.hasInitializedTarget = true;
	}

	DirectX::XMFLOAT3 targetPos = footSettings.targetPos;

	// 足首の少し上からまっすぐ下へレイを飛ばし、地面の法線を取得する
	// endPosはモデル空間の座標なので、レイを飛ばす前にワールド空間へ変換する
	DirectX::XMVECTOR endPosModel = DirectX::XMLoadFloat3(&endPos);
	DirectX::XMMATRIX entityWorld = DirectX::XMLoadFloat4x4(&entityWorldMatrix);
	DirectX::XMVECTOR endPosWorldV = DirectX::XMVector3TransformCoord(endPosModel, entityWorld);

	DirectX::XMFLOAT3 rayOrigin;
	DirectX::XMStoreFloat3(&rayOrigin, endPosWorldV);
	rayOrigin.y += footSettings.rayStartOffset; // めり込まないように少し上げる
	DirectX::XMFLOAT3 rayDirection = { 0.0f, -1.0f, 0.0f };
	float rayMaxDistance = footSettings.rayMaxDistance;

	// レイを飛ばす
	RaycastHitResult groundHit = PhysicsSystem::Raycast(
		pxScene, rayOrigin, rayDirection, rayMaxDistance, PhysicsLayer::Static);

	if (groundHit.hit)
	{
		// 地面から足首までの高さ分、地面の当たり位置より持ち上げる(めり込み防止)
		DirectX::XMFLOAT3 hitPos = groundHit.position;
		hitPos.y += footSettings.ankleHeight;

		// groundHit.positionはワールド座標なので、モデル空間に変換してからtargetPosとして使う
		// (地面が見つからなかった場合は、targetPosは最初に代入したfootSettings.targetPosのまま)
		DirectX::XMVECTOR hitPosWorldV = DirectX::XMLoadFloat3(&hitPos);
		DirectX::XMMATRIX entityWorldInverse = DirectX::XMMatrixInverse(nullptr, entityWorld);
		DirectX::XMVECTOR hitPosModelV = DirectX::XMVector3TransformCoord(hitPosWorldV, entityWorldInverse);
		DirectX::XMStoreFloat3(&targetPos, hitPosModelV);
	}

	// poleVector: 膝を前方(モデルのZ+方向を仮定)に向ける
	DirectX::XMFLOAT3 poleVector = rootPos;
	poleVector.z += footSettings.poleVectorForward ? -1.0f : 1.0f;

	// 2BoneIKを解く
	TwoBoneIKSolver::Result result = TwoBoneIKSolver::Solve(
		rootPos, midPos, endPos,
		rootRot, midRot,
		targetPos, poleVector);

	// ワールド回転 → ローカル回転 に変換して書き戻す
	// rootの親(pelvis)のワールド回転はまだ変わっていないので、そのまま使える
	DirectX::XMVECTOR pelvisWorldRot;
	{
		DirectX::XMFLOAT3 dummyPos;
		DirectX::XMFLOAT4 pelvisRot;
		int pelvisIndex = skinned.model->GetNodes()[rootIndex].parentIndex;
		DecomposePosRot(globals[pelvisIndex], dummyPos, pelvisRot);
		pelvisWorldRot = DirectX::XMLoadFloat4(&pelvisRot);
	}

	DirectX::XMVECTOR newRootLocal = WorldToLocalRotation(
		DirectX::XMLoadFloat4(&result.rootRotation), pelvisWorldRot);
	DirectX::XMVECTOR newMidLocal = WorldToLocalRotation(
		DirectX::XMLoadFloat4(&result.midRotation), DirectX::XMLoadFloat4(&result.rootRotation));

	DirectX::XMStoreFloat4(&skinned.localRotations[rootIndex], newRootLocal);
	DirectX::XMStoreFloat4(&skinned.localRotations[midIndex], newMidLocal);

	// 足首: 地面の法線に合わせて、水平からのズレ分だけ元の向き(endRot)に上乗せする
	DirectX::XMVECTOR groundUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	DirectX::XMVECTOR groundDeltaRotation = groundHit.hit
		? TwoBoneIKSolver::RotationFromToRotation(groundUp, DirectX::XMLoadFloat3(&groundHit.normal))
		: DirectX::XMQuaternionIdentity(); // 地面が見つからなければ、元の向きのまま何もしない

	DirectX::XMVECTOR newEndWorld = DirectX::XMQuaternionMultiply(DirectX::XMLoadFloat4(&endRot), groundDeltaRotation);
	DirectX::XMVECTOR newEndLocal = WorldToLocalRotation(
		newEndWorld, DirectX::XMLoadFloat4(&result.midRotation));

	DirectX::XMStoreFloat4(&skinned.localRotations[endIndex], newEndLocal);

	// IKで回転が変わったので、グローバル行列を再計算する
	SkeletonUtils::CalcGlobalTransforms(
		skinned.model->GetNodes(),
		skinned.localPositions, skinned.localRotations, skinned.localScales,
		skinned.nodeGlobalTransforms);

	SkeletonUtils::CalcSkinningMatrices(
		*skinned.model, skinned.nodeGlobalTransforms, skinned.boneTransforms);
}