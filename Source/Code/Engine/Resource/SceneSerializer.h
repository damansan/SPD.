#pragma once
#include <functional>
#include <string>
#include "entt.hpp"
#include <json.hpp>

// シーン保存 / 読み込み
class SceneSerializer
{
public:
	// 名前はコールバックで受け取る
	using NameGetter = std::function<std::string(entt::entity)>;

	static constexpr int Version = 1;

	// シーン全体をJSONにする
	static nlohmann::json Serialize(entt::registry& registry, const NameGetter& getName = nullptr);

	// JSONをファイルに書き出す
	static bool SaveToFile(entt::registry& registry, const std::string& path, const NameGetter& getName = nullptr);

	// 名前はエディタ側の情報なので、設定はコールバックで渡す
	using NameSetter = std::function<void(entt::entity, const std::string&)>;

	// JSONの内容で、Entityを作る（既存のEntityには触れない）
	static void Deserialize(entt::registry& registry, const nlohmann::json& scene, const NameSetter& setName = nullptr);

	// ファイルから読み込む。読み込み前に、既存のEntityを1つずつdestroyする
	static bool LoadFromFile(entt::registry& registry, const std::string& path, const NameSetter& setName = nullptr);

private:
	// 現在のEntityの値とプレハブでの値を比較する
	static nlohmann::json DiffAgainstPrefab(const nlohmann::json& current, const nlohmann::json& prefab);

};