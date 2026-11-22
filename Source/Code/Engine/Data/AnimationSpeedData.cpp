#include "AnimationSpeedData.h"
#include <algorithm>

// 指定したクリップ名のデータを探す
AnimationSpeedClipData* AnimationSpeedData::FindOrCreateClip(const std::string& clipName)
{
	for (auto& clip : clips)
	{
		if (clip.clipName == clipName) return &clip;
	}

	AnimationSpeedClipData newClip;
	newClip.clipName = clipName;

	// 始端/終端の固定ポイントを1.0倍で配置しておく
	AnimationSpeedPoint startPoint;
	startPoint.id = newClip.GenerateId();
	startPoint.time = 0.0f;
	startPoint.speed = 1.0f;
	startPoint.isEndpoint = true;
	newClip.points.push_back(startPoint);

	AnimationSpeedPoint endPoint;
	endPoint.id = newClip.GenerateId();
	endPoint.time = 1.0f;
	endPoint.speed = 1.0f;
	endPoint.isEndpoint = true;
	newClip.points.push_back(endPoint);

	clips.push_back(newClip);
	return &clips.back();
}

// 正規化時間における速度を線形補完で求める
float AnimationSpeedClipData::EvaluateSpeed(float normalizedTime) const
{
	if (points.empty()) return 1.0f;

	// ソート用一時配列
	std::vector<const AnimationSpeedPoint*> sorted;
	sorted.reserve(points.size());
	for (const auto& p : points) sorted.push_back(&p);
	std::sort(sorted.begin(), sorted.end(),
		[](const AnimationSpeedPoint* a, const AnimationSpeedPoint* b) { return a->time < b->time; });

	// 両端
	if (normalizedTime <= sorted.front()->time) return sorted.front()->speed;
	if (normalizedTime >= sorted.back()->time)  return sorted.back()->speed;

	// 挟んでいる2点を探して線形補間
	for (size_t i = 0; i + 1 < sorted.size(); ++i)
	{
		const AnimationSpeedPoint* a = sorted[i];
		const AnimationSpeedPoint* b = sorted[i + 1];
		if (normalizedTime >= a->time && normalizedTime <= b->time)
		{
			float range = b->time - a->time;
			float t = (range > 0.0f) ? (normalizedTime - a->time) / range : 0.0f;
			return a->speed + (b->speed - a->speed) * t;
		}
	}

	return 1.0f;
}

// 保存
nlohmann::ordered_json AnimationSpeedData::ToJson() const
{
	nlohmann::ordered_json j;
	j["clips"] = nlohmann::json::array();

	for (const auto& clip : clips)
	{
		nlohmann::ordered_json jc;
		jc["clipName"] = clip.clipName;
		jc["nextId"] = clip.nextId;

		jc["points"] = nlohmann::json::array();
		for (const auto& p : clip.points)
		{
			nlohmann::ordered_json jp;
			jp["id"] = p.id;
			jp["time"] = p.time;
			jp["speed"] = p.speed;
			jp["isEndpoint"] = p.isEndpoint;
			jc["points"].push_back(jp);
		}

		j["clips"].push_back(jc);
	}

	return j;
}

// 読み込み
void AnimationSpeedData::FromJson(const nlohmann::json& j)
{
	clips.clear();

	if (!j.contains("clips")) return;

	for (const auto& jc : j["clips"])
	{
		AnimationSpeedClipData clip;
		clip.clipName = jc.value("clipName", "");
		clip.nextId = jc.value("nextId", 1);

		if (jc.contains("points"))
		{
			for (const auto& jp : jc["points"])
			{
				AnimationSpeedPoint p;
				p.id = jp.value("id", -1);
				p.time = jp.value("time", 0.0f);
				p.speed = jp.value("speed", 1.0f);
				p.isEndpoint = jp.value("isEndpoint", false);
				clip.points.push_back(p);
			}
		}

		clips.push_back(clip);
	}
}