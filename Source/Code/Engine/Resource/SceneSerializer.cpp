#include "SceneSerializer.h"
#include "Engine/Resource/EntitySerializer.h"
#include "Engine/ECS/Components/PrefabInstanceComponent.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/JsonFileCache.h"
#include <algorithm>
#include <filesystem>
#include <vector>

nlohmann::json SceneSerializer::Serialize(entt::registry& registry, const NameGetter& getName)
{
	nlohmann::json scene = nlohmann::json::object();
	scene["version"] = Version;
	scene["settings"] = nlohmann::json::object(); // 環境設定（ライト・SkyBoxなど）の置き場所。今は空

	std::vector<entt::entity> entities;
	for (auto e : registry.view<entt::entity>())
	{
		entities.push_back(e);
	}
	std::reverse(entities.begin(), entities.end());

	nlohmann::json entries = nlohmann::json::array();
	for (auto e : entities)
	{
		nlohmann::json entry = nlohmann::json::object();

		if (getName)
		{
			entry["name"] = getName(e);
		}

		if (auto* prefab = registry.try_get<PrefabInstanceComponent>(e))
		{
			// プレハブ配置のEntityは、現在値とプレハブの値の差分だけを書く
			entry["prefab"] = prefab->prefabPath;

			if (const nlohmann::json* prefabData = EntitySerializer::LoadPrefabFile(prefab->prefabPath))
			{
				nlohmann::json current = EntitySerializer::SerializeEntity(registry, e);
				nlohmann::json overrides = DiffAgainstPrefab(current, *prefabData);

				if (!overrides.empty())
				{
					entry["overrides"] = std::move(overrides);
				}
			}
			else
			{
				// プレハブ本体が読めない場合は、差分を取れないので、参照だけ残す
				Log("[Warn] 差分比較用のプレハブを読めない: %s", LogPath(prefab->prefabPath).c_str());
			}
		}
		else
		{
			// それ以外は、プレハブと同じ形式で、コンポーネントの中身を書く
			entry["components"] = EntitySerializer::SerializeEntity(registry, e);
		}

		entries.push_back(std::move(entry));
	}
	scene["entities"] = std::move(entries);

	return scene;
}

bool SceneSerializer::SaveToFile(entt::registry& registry, const std::string& path, const NameGetter& getName)
{
	nlohmann::json scene = Serialize(registry, getName);

	// シーンは使い回さないので、その場限りで使う
	JsonFileCache<nlohmann::json> file;
	if (!file.Save(path, scene, [](const nlohmann::json& d) { return nlohmann::ordered_json(d); }))
	{
		return false;
	}

	Log("[Info] シーンを保存: %s (Entity %d 個)", LogPath(path).c_str(), static_cast<int>(scene["entities"].size()));
	return true;
}

// JSONの内容で、Entityを作る
void SceneSerializer::Deserialize(entt::registry& registry, const nlohmann::json& scene, const NameSetter& setName)
{
	if (!scene.contains("entities") || !scene["entities"].is_array())
	{
		Log("シーンの形式が不正です(entitiesがありません)");
		return;
	}

	int failedCount = 0;

	for (const auto& entry : scene["entities"])
	{
		entt::entity e = entt::null;

		if (entry.contains("prefab") && entry["prefab"].is_string())
		{
			e = EntitySerializer::InstantiatePrefab(registry, entry["prefab"].get<std::string>());
		}
		else if (entry.contains("components"))
		{
			e = EntitySerializer::CreateFromComponents(registry, entry["components"]);
		}

		if (e == entt::null)
		{
			// InstantiatePrefab / CreateFromComponents 側が、詳しい理由をLogに出している
			++failedCount;
			continue;
		}

		// プレハブ生成の後、差分があれば上書きする
		if (entry.contains("overrides") && entry["overrides"].is_object())
		{
			try
			{
				EntitySerializer::DeserializeEntity(registry, e, entry["overrides"]);
			}
			catch (const nlohmann::json::exception&)
			{
				Log("[Warn] overrides の適用に失敗: %s", entry.value("name", std::string()).c_str());
			}
		}

		if (setName && entry.contains("name") && entry["name"].is_string())
		{
			setName(e, entry["name"].get<std::string>());
		}
	}

	if (failedCount > 0)
	{
		Log("%d個のEntityを復元できませんでした", failedCount);
	}
}

// ファイルから読み込む。読み込み前に、既存のEntityを1つずつdestroyする
bool SceneSerializer::LoadFromFile(entt::registry& registry, const std::string& path, const NameSetter& setName)
{
	// シーンは開くときに1回読むだけなので、その場限りで使う
	JsonFileCache<nlohmann::json> file;
	const nlohmann::json& scene = file.GetOrLoad(path,
		[](nlohmann::json& d, const nlohmann::json& j) { d = j; });
	if (file.GetStatus(path) != JsonFileStatus::Loaded) return false;

	// 既存のEntityを、1つずつ破棄する
	std::vector<entt::entity> existing;
	for (auto e : registry.view<entt::entity>())
	{
		existing.push_back(e);
	}
	for (auto e : existing)
	{
		registry.destroy(e);
	}

	Deserialize(registry, scene, setName);

	Log("シーンを読み込み: %s", LogPath(path).c_str());
	return true;
}

// 現在のEntityの値とプレハブでの値を比較する
nlohmann::json SceneSerializer::DiffAgainstPrefab(const nlohmann::json& current, const nlohmann::json& prefab)
{
	nlohmann::json overrides = nlohmann::json::object();

	// currentは、そのEntityが今持っているコンポーネントだけを含む
	for (auto& [componentName, currentFields] : current.items())
	{
		if (!currentFields.is_object()) continue;

		// プレハブ側に無いコンポーネント（配置後に追加した）は、比較のしようがないので対象外
		if (!prefab.contains(componentName)) continue;
		const nlohmann::json& prefabFields = prefab[componentName];
		if (!prefabFields.is_object()) continue;

		nlohmann::json fieldDiff = nlohmann::json::object();

		for (auto& [fieldName, value] : currentFields.items())
		{
			// プレハブ側に無いフィールド（改名・追加）は、比較できないので飛ばす
			if (!prefabFields.contains(fieldName)) continue;

			if (value != prefabFields[fieldName])
			{
				fieldDiff[fieldName] = value;
			}
		}

		if (!fieldDiff.empty())
		{
			overrides[componentName] = std::move(fieldDiff);
		}
	}

	return overrides;
}
