#pragma once
#include "entt.hpp"
#include <json.hpp>

class EntitySerializer
{
public:
	// Entity1つを、コンポーネント名をキーにしたJSONへ変換する
	static nlohmann::json SerializeEntity(entt::registry& registry, entt::entity e);

	// JSONの内容を、Entityへ反映する
	static void DeserializeEntity(entt::registry& registry, entt::entity e, const nlohmann::json& data);

	// プレハブファイルを読む
	static const nlohmann::json* LoadPrefabFile(const std::string& path);
	// プレハブファイルに書く
	static bool SavePrefabFile(const std::string& path, const nlohmann::json& data);

	// プレハブファイルからEntityを生成する。成功したらそのEntityを、失敗したらentt::nullを返す
	static entt::entity InstantiatePrefab(entt::registry& registry, const std::string& prefabPath);

	// コンポーネントの中身(JSON)からEntityを生成する。復元中にエラーが起きたら、作りかけのEntityを破棄してentt::nullを返す
	static entt::entity CreateFromComponents(entt::registry& registry, const nlohmann::json& components);

private:
	// entt::meta_any 1つ分のフィールド値を、JSONへ変換する
	static nlohmann::json SerializeMetaAny(entt::meta_any& value);
	// JSONの値を、entt::meta経由でフィールドへ書き戻す
	static void DeserializeMetaAny(entt::meta_data metaData, entt::meta_any& instance, const nlohmann::json& value);
};