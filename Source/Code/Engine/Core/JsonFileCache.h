#pragma once
#include "Engine/Core/Log.h"

#include <string>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <filesystem>
#include <json.hpp>

// 読み込み結果
enum class JsonFileStatus
{
	Loaded,  // 読み込み済み
	Missing, // ファイルなし
	Broken,  // 読み込めない
};

// ファイルが無いときにログを出すか
enum class JsonMissingLog
{
	Silent, // 無くても支障が無いもの
	Report, // 無いとおかしいもの
};

template<typename T>
class JsonFileCache
{
public:
	using LoadFunc = std::function<void(T&, const nlohmann::json&)>;
	using SaveFunc = std::function<nlohmann::ordered_json(const T&)>; // キーの順番を保つ

	// 読む
	T& GetOrLoad(const std::string& path, const LoadFunc& loadFunc, JsonMissingLog missingLog = JsonMissingLog::Report)
	{
		return Find(path, loadFunc, missingLog).data;
	}

	// 最後に読んだときの結果
	JsonFileStatus GetStatus(const std::string& path) const
	{
		auto it = cache.find(path);
		return (it != cache.end()) ? it->second.status : JsonFileStatus::Missing;
	}

	// 書く
	bool Save(const std::string& path, const T& data, const SaveFunc& saveFunc)
	{
		namespace fs = std::filesystem;
		std::error_code ec;
		fs::create_directories(fs::path(path).parent_path(), ec);

		// 一時ファイルに書き切る
		const std::string tempPath = path + ".tmp";
		{
			std::ofstream ofs(tempPath);
			if (!ofs.is_open())
			{
				Log(" 保存できない: %s", LogPath(path).c_str());
				return false;
			}
			ofs << saveFunc(data).dump(4, ' ', false, nlohmann::ordered_json::error_handler_t::replace);
			if (!ofs)
			{
				Log(" 保存できない: %s", LogPath(path).c_str());
				return false;
			}
		}

		// 本物と置き換える
		fs::rename(tempPath, path, ec);
		if (ec)
		{
			Log(" 保存できない: %s", LogPath(path).c_str());
			fs::remove(tempPath, ec);
			return false;
		}

		// 自分のキャッシュも書いた内容にしておく
		Entry& entry = cache[path];
		entry.data = data;
		entry.lastWriteTime = fs::last_write_time(path, ec);
		entry.exists = !ec;
		entry.status = JsonFileStatus::Loaded;
		return true;
	}

private:
	struct Entry
	{
		T data;
		std::filesystem::file_time_type lastWriteTime;
		bool exists = false;                             // 読んだときにファイルがあったか
		JsonFileStatus status = JsonFileStatus::Missing; // 読んだ結果
	};

	// キャッシュから探す。ファイルの状態が変わっていれば読み直す
	Entry& Find(const std::string& path, const LoadFunc& loadFunc, JsonMissingLog missingLog)
	{
		std::error_code ec;
		const auto writeTime = std::filesystem::last_write_time(path, ec);
		const bool exists = !ec;

		auto it = cache.find(path);
		if (it != cache.end())
		{
			Entry& cached = it->second;

			// 前回と同じ状態なら、キャッシュをそのまま返す
			if (exists == cached.exists && (!exists || writeTime == cached.lastWriteTime))
			{
				return cached;
			}
			// ファイルが更新された・現れた・消えた場合は、読み直す
			cache.erase(it);
		}

		// マップの中に直接作ってから読む
		Entry& entry = cache.try_emplace(path).first->second;
		entry.lastWriteTime = writeTime;
		entry.exists = exists;

		// 状態が変わったときだけここに来るので、ログは1回だけ出る
		if (!exists)
		{
			entry.status = JsonFileStatus::Missing;
			if (missingLog == JsonMissingLog::Report)
			{
				Log(" ファイルが見つからない: %s", LogPath(path).c_str());
			}
			return entry;
		}

		std::ifstream ifs(path);
		if (!ifs.is_open())
		{
			entry.status = JsonFileStatus::Broken;
			Log(" ファイルを開けない: %s", LogPath(path).c_str());
			return entry;
		}

		try
		{
			nlohmann::json j;
			ifs >> j;
			loadFunc(entry.data, j);
			entry.status = JsonFileStatus::Loaded;
		}
		catch (const std::exception&)
		{
			// 途中まで読んだ分は捨てて、初期値にする
			entry.data = T{};
			entry.status = JsonFileStatus::Broken;
			Log(" JSON が読めない: %s", LogPath(path).c_str());
		}

		return entry;
	}

	std::unordered_map<std::string, Entry> cache;
};