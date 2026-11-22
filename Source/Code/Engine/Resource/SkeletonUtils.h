#pragma once
#include <vector>
#include <DirectXMath.h>
#include "Model.h"

// モデルの骨格階層を扱う共通計算ロジック
namespace SkeletonUtils
{
	// ワールド空間のグローバル行列を再帰計算する
	void CalcGlobalTransforms(
		const std::vector<Model::Node>& nodes,
		const std::vector<DirectX::XMFLOAT3>& localPositions,
		const std::vector<DirectX::XMFLOAT4>& localRotations,
		const std::vector<DirectX::XMFLOAT3>& localScales,
		std::vector<DirectX::XMFLOAT4X4>& outGlobalTransforms);

	// グローバル行列とBone.offsetTransformから、スキニング用のボーン行列を求める
	void CalcSkinningMatrices(
		const Model& model,
		const std::vector<DirectX::XMFLOAT4X4>& globalTransforms,
		std::vector<std::vector<DirectX::XMFLOAT4X4>>& outBoneTransforms);
}