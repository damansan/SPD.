#pragma once
#include <algorithm>
#include <filesystem>
#include <string>

// データファイルの種類
enum class DataFileKind
{
	Unknown,    // データファイルではない
	Scene,      // .scene.json
	Prefab,     // .prefab.json
	StateGraph, // .stategraph.json
	AnimEdit,   // .animedit.json
	Sequencer,  // .sequencer.json
};

// 種類ごとの拡張子
inline const char* GetDataFileExtension(DataFileKind kind)
{
	switch (kind)
	{
	case DataFileKind::Scene:      return ".scene.json";
	case DataFileKind::Prefab:     return ".prefab.json";
	case DataFileKind::StateGraph: return ".stategraph.json";
	case DataFileKind::AnimEdit:   return ".animedit.json";
	case DataFileKind::Sequencer:  return ".sequencer.json";
	default:                       return "";
	}
}

// パスから種類を判定する
inline DataFileKind GetDataFileKind(const std::filesystem::path& path)
{
	// 大文字小文字を区別しない
	std::string name = path.filename().string();
	std::transform(name.begin(), name.end(), name.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	constexpr DataFileKind kinds[] =
	{
		DataFileKind::Scene,
		DataFileKind::Prefab,
		DataFileKind::StateGraph,
		DataFileKind::AnimEdit,
		DataFileKind::Sequencer,
	};

	for (DataFileKind kind : kinds)
	{
		const std::string extension = GetDataFileExtension(kind);

		// 「.scene.json」だけの名前（中身の名前が空）は、データファイルとみなさない
		if (name.size() > extension.size()
			&& name.compare(name.size() - extension.size(), extension.size(), extension) == 0)
		{
			return kind;
		}
	}
	return DataFileKind::Unknown;
}


// モデルに属するデータの場所：モデルと同じフォルダの Json/<モデル名>.種類.json
inline std::string GetModelDataFilePath(const std::string& modelPath, DataFileKind kind)
{
	const std::filesystem::path p(modelPath);
	const std::filesystem::path result = p.parent_path() / "Json" / (p.stem().string() + GetDataFileExtension(kind));
	return result.generic_string(); // 区切りは「/」にそろえる
}