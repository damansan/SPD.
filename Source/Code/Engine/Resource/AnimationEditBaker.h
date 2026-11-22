#pragma once
#include <string>
#include <vector>
#include "Engine/Resource/Model.h"
#include "Engine/Data/AnimationEditData.h"

// アニメーションを編集する
class AnimationEditBaker
{
public:
	static std::vector<Model::Animation> Bake(
		const std::vector<Model::Animation>& rawAnimations,
		const std::vector<Model::Node>& nodes,
		const AnimationEditData& editData);

	// 1クリップ分だけ焼く
	static Model::Animation BakeClip(
		const Model::Animation& rawAnimation,
		const std::vector<Model::Node>& nodes,
		int rootIndex,
		const AnimationEditClipData* clip);

	// RootNodeの名前を探す
	static std::string FindDefaultRootNodeName(const Model& model);
	// スキンのボーンになっているノードの番号の一覧
	static std::vector<int> CollectBoneIndices(const Model& model);
	// 名前からルートノードの番号を探す
	static int FindNodeIndex(const std::vector<Model::Node>& nodes, const std::string& name);

private:

	// 指定範囲だけ残して、時間を0秒始まりに詰める
	static void TrimAnimation(Model::Animation& animation, float start, float end);

	// 0フレーム目のルート位置が、バインドポーズの位置になるようにずらす
	static void RemoveRootOffset(Model::NodeAnim& rootAnim, const Model::Node& rootNode);

	// 区間ごとに、ルートの移動量に倍率を掛ける
	static void ApplySections(
		Model::Animation& animation,
		int rootIndex,
		const std::vector<AnimationEditSection>& sectionsXZ,
		const std::vector<AnimationEditSection>& sectionsY);

	// ルートをY軸回りに回す
	static void ApplyYawOffset(Model::NodeAnim& rootAnim, const Model::Node& rootNode, float yawDegrees);
};