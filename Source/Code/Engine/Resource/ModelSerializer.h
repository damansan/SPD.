#pragma once
#include <memory>
#include <string>

// 前方宣言
class Model;

namespace cereal
{
	class BinaryOutputArchive;
	class BinaryInputArchive;
}

class ModelSerializer
{
public:
	// Modelをバイナリファイルに保存する
	static void Save(const Model& model, const std::string& filepath);

	// バイナリファイルからModelを復元する
	static void Load(Model& model, const std::string& filepath);

	// cerealファイルが存在するか確認
	static bool Exists(const std::string& gltfPath);

	// 拡張子を .cereal に置き換えたパスを返す
	static std::string ToCerealPath(const std::string& gltfPath);
};