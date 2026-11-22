#include "SequencerData.h"
#include "Engine/Core/Log.h"
#include "Engine/Data/DataFileKind.h"
#include <filesystem>

namespace
{
	std::string ToString(SequencerFlag flag)
	{
		switch (flag)
		{
		case SequencerFlag::AnimExit: return "AnimExit";
		default: break;
		}
		return "AnimExit";
	}

	// 名前からフラグに直す
	bool TryParseSequencerFlag(const std::string& s, SequencerFlag& outFlag)
	{
		if (s == "AnimExit") { outFlag = SequencerFlag::AnimExit; return true; }
		return false;
	}
}


// 指定したクリップ名のデータを探す
const SequencerClipData* SequencerData::FindClip(const std::string& clipName) const
{
	for (const auto& clip : clips)
	{
		if (clip.clipName == clipName) return &clip;
	}
	return nullptr;
}
SequencerClipData* SequencerData::FindClip(const std::string& clipName)
{
	return const_cast<SequencerClipData*>(static_cast<const SequencerData&>(*this).FindClip(clipName));
}
SequencerClipData* SequencerData::FindOrCreateClip(const std::string& clipName)
{
	if (SequencerClipData* clip = FindClip(clipName)) return clip;

	SequencerClipData newClip;
	newClip.clipName = clipName;
	clips.push_back(newClip);
	return &clips.back();
}

// 保存
nlohmann::ordered_json SequencerData::ToJson() const
{
	nlohmann::ordered_json j;
	j["clips"] = nlohmann::json::array();

	for (const auto& clip : clips)
	{
		nlohmann::ordered_json jc;
		jc["clipName"] = clip.clipName;
		jc["nextId"] = clip.nextId;

		jc["ranges"] = nlohmann::json::array();
		for (const auto& r : clip.ranges)
		{
			nlohmann::ordered_json jr;
			jr["id"] = r.id;
			jr["start"] = r.start;
			jr["end"] = r.end;
			jr["flag"] = ToString(r.flag);
			jc["ranges"].push_back(jr);
		}

		jc["points"] = nlohmann::json::array();
		for (const auto& p : clip.points)
		{
			nlohmann::ordered_json jp;
			jp["id"] = p.id;
			jp["time"] = p.time;
			jp["flag"] = ToString(p.flag);
			jc["points"].push_back(jp);
		}

		j["clips"].push_back(jc);
	}

	return j;
}

// 読み込み
void SequencerData::FromJson(const nlohmann::json& j)
{
	clips.clear();

	if (!j.contains("clips")) return;

	for (const auto& jc : j["clips"])
	{
		SequencerClipData clip;
		clip.clipName = jc.value("clipName", "");
		clip.nextId = jc.value("nextId", 1);

		if (jc.contains("ranges"))
		{
			for (const auto& jr : jc["ranges"])
			{
				SequencerRange r;
				r.id = jr.value("id", -1);
				r.start = jr.value("start", 0.0f);
				r.end = jr.value("end", 0.0f);

				// 知らないフラグ名は読み飛ばす
				const std::string flagName = jr.value("flag", "");
				if (!TryParseSequencerFlag(flagName, r.flag))
				{
					Log(" Sequencer: 不明なフラグ「%s」", flagName.c_str());
					continue;
				}
				clip.ranges.push_back(r);
			}
		}

		if (jc.contains("points"))
		{
			for (const auto& jp : jc["points"])
			{
				SequencerPoint p;
				p.id = jp.value("id", -1);
				p.time = jp.value("time", 0.0f);

				// 知らないフラグ名は読み飛ばす
				const std::string flagName = jp.value("flag", "");
				if (!TryParseSequencerFlag(flagName, p.flag))
				{
					Log(" Sequencer: 不明なフラグ「%s」", flagName.c_str());
					continue;
				}
				clip.points.push_back(p);
			}
		}

		clips.push_back(clip);
	}
}

// モデルパスから保存先のパスを組み立てる
std::string SequencerData::GetFilePath(const std::string& modelPath)
{
	return GetModelDataFilePath(modelPath, DataFileKind::Sequencer);
}