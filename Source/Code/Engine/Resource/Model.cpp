#include "Model.h"
#include "Engine/Resource/AnimationEditBaker.h"
#include <algorithm>

namespace
{
	// 対応表を使って、片方の時刻をもう片方に直す
	float MapTime(const std::vector<float>& from, const std::vector<float>& to, float t)
	{
		// 対応表が無ければ、同じ時刻
		if (from.empty() || from.size() != to.size()) return t;

		if (t <= from.front()) return to.front();

		for (size_t i = 1; i < from.size(); i++)
		{
			if (t <= from[i])
			{
				const float span = from[i] - from[i - 1];
				const float rate = (span > 0.0f) ? (t - from[i - 1]) / span : 0.0f;
				return to[i - 1] + (to[i] - to[i - 1]) * rate;
			}
		}
		return to.back();
	}

	// クリップの並びから、名前で探す
	const Model::Animation* FindByName(const std::vector<Model::Animation>& animations, const std::string& name)
	{
		for (const auto& anim : animations)
		{
			if (anim.name == name) return &anim;
		}
		return nullptr;
	}
}


// 入力レイアウト定義
const std::vector<D3D11_INPUT_ELEMENT_DESC> Model::InputElementDescs =
{
    { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TANGENT",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "WEIGHTS",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "BONES",      0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

// 名前からクリップを探す
const Model::Animation* Model::FindAnimation(const std::string& name) const
{
	return FindByName(animations, name);
}
const Model::Animation* Model::FindRawAnimation(const std::string& name) const
{
	return FindByName(rawAnimations, name);
}
const Model::Animation* Model::FindPreviewAnimation(const std::string& name) const
{
	return FindByName(previewAnimations, name);
}

// 編集設定をプレビュー用に適用する
void Model::ApplyAnimationEditPreview(const AnimationEditData& editData)
{
	EnsureRawAnimations();

	// カットだけを外す
	AnimationEditData preview = editData;
	for (auto& clip : preview.clips)
	{
		clip.trimStart = 0.0f;
		clip.trimEnd = -1.0f;
	}

	previewAnimations = AnimationEditBaker::Bake(rawAnimations, nodes, preview);
}
// プレビュー用を消す
void Model::ClearAnimationPreview()
{
	previewAnimations.clear();
}

// 再生に使うクリップの並び
const std::vector<Model::Animation>& Model::GetPlaybackAnimations(bool usePreview) const
{
	return (usePreview && !previewAnimations.empty()) ? previewAnimations : animations;
}

// 編集設定を焼き込む
void Model::ApplyAnimationEdit(const AnimationEditData& editData)
{
	EnsureRawAnimations();

    animations = AnimationEditBaker::Bake(rawAnimations, nodes, editData);

	// 焼き込みと同じボーンを、ランタイムでも固定するために覚えておく
	rootNodeName = editData.rootNodeName;
	rootNodeIndex = AnimationEditBaker::FindNodeIndex(nodes, rootNodeName);
}

// 生データがまだ控えられていなければ、今のアニメーションを控える
void Model::EnsureRawAnimations()
{
	// 初回の焼き込みより前なら、animations はまだ加工されていない生データ
	if (rawAnimations.empty())
	{
		rawAnimations = animations;
	}
}

// 焼き込み後から生データに
float Model::Animation::ToRawTime(float bakedTime) const
{
	return MapTime(bakedTimes, rawTimes, bakedTime);
}
// 生データから焼き込み後に
float Model::Animation::ToBakedTime(float rawTime) const
{
	return MapTime(rawTimes, bakedTimes, rawTime);
}

// 1フレームの秒数
float Model::Animation::CalcFrameStep() const
{
	size_t maxKeys = 0;
	for (const auto& nodeAnim : nodeAnims)
	{
		maxKeys = (std::max)(maxKeys, nodeAnim.positionKeyframes.size());
		maxKeys = (std::max)(maxKeys, nodeAnim.rotationKeyframes.size());
	}

	if (maxKeys < 2 || secondsLength <= 0.0f) return 0.0f;
	return secondsLength / (float)(maxKeys - 1);
}
