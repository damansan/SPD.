#include "SkeletonUtils.h"
#include <functional>

namespace SkeletonUtils
{
	// 親子関係を辿り、各ノードのローカルpose(position/rotation/scale)から
	// ワールド空間のグローバル行列を再帰計算する
	void CalcGlobalTransforms(
		const std::vector<Model::Node>& nodes,
		const std::vector<DirectX::XMFLOAT3>& localPositions,
		const std::vector<DirectX::XMFLOAT4>& localRotations,
		const std::vector<DirectX::XMFLOAT3>& localScales,
		std::vector<DirectX::XMFLOAT4X4>& outGlobalTransforms)
	{
		outGlobalTransforms.resize(nodes.size());

		// ルートから再帰的にグローバル行列を計算
		std::function<void(int, const DirectX::XMFLOAT4X4&)> calcGlobal =
			[&](int nodeIndex, const DirectX::XMFLOAT4X4& parentTransform)
			{
				const auto& node = nodes[nodeIndex];

				const auto& position = localPositions[nodeIndex];
				const auto& rotation = localRotations[nodeIndex];
				const auto& scale = localScales[nodeIndex];

				// ローカル行列を計算
				DirectX::XMMATRIX S = DirectX::XMMatrixScaling(
					scale.x, scale.y, scale.z);
				DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(
					DirectX::XMVectorSet(
						rotation.x, rotation.y,
						rotation.z, rotation.w));
				DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
					position.x, position.y, position.z);

				DirectX::XMMATRIX local = S * R * T;
				DirectX::XMMATRIX parent = DirectX::XMLoadFloat4x4(&parentTransform);
				DirectX::XMMATRIX global = local * parent;

				DirectX::XMStoreFloat4x4(&outGlobalTransforms[nodeIndex], global);

				// 子ノードを再帰処理
				for (int child : node.childIndices)
				{
					calcGlobal(child, outGlobalTransforms[nodeIndex]);
				}
			};

		// ルートノードから計算開始
		DirectX::XMFLOAT4X4 identity =
		{ 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

		for (int i = 0; i < (int)nodes.size(); i++)
		{
			if (nodes[i].parentIndex < 0)
			{
				calcGlobal(i, identity);
			}
		}
	}

	// グローバル行列とBone.offsetTransformから、スキニング用のボーン行列を求める
	void CalcSkinningMatrices(
		const Model& model,
		const std::vector<DirectX::XMFLOAT4X4>& globalTransforms,
		std::vector<std::vector<DirectX::XMFLOAT4X4>>& outBoneTransforms)
	{
		const auto& meshes = model.GetMeshes();
		if (meshes.empty()) return;

		outBoneTransforms.resize(meshes.size());

		for (int m = 0; m < (int)meshes.size(); m++)
		{
			const auto& mesh = meshes[m];
			outBoneTransforms[m].resize(mesh.bones.size());

			for (int i = 0; i < (int)mesh.bones.size(); i++)
			{
				const auto& bone = mesh.bones[i];
				int nodeIdx = bone.nodeIndex;
				if (nodeIdx < 0 || nodeIdx >= (int)globalTransforms.size()) continue;

				DirectX::XMMATRIX offset = DirectX::XMLoadFloat4x4(&bone.offsetTransform);
				DirectX::XMMATRIX global = DirectX::XMLoadFloat4x4(&globalTransforms[nodeIdx]);

				DirectX::XMStoreFloat4x4(&outBoneTransforms[m][i], offset * global);
			}
		}
	}
}