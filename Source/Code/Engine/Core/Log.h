#pragma once
#include <Windows.h>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <mutex>
#include <string>
#include <vector>
#include <filesystem>

// ログの保存先。メッセージはUTF-8で持つ（日本語は "..." を付けて書く）
namespace LogBuffer
{
	struct Entry
	{
		std::string time;    // "HH:MM:SS"
		std::string message; // UTF-8
	};

	namespace detail
	{
		struct Storage
		{
			std::mutex        mutex;
			std::deque<Entry> entries;
			uint64_t          version = 0; // 書き込みのたびに増える（表示側が更新を検知する）
		};

		// 関数内staticにして、起動時の初期化順の問題を避ける
		inline Storage& GetStorage()
		{
			static Storage storage;
			return storage;
		}

		inline std::wstring Utf8ToWide(const std::string& text)
		{
			int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
			if (length <= 0) return std::wstring();

			std::wstring result(length, L'\0');
			MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), length);
			return result;
		}
	}

	constexpr size_t MaxEntries = 2000; // これを超えたら、古いものから捨てる

	inline void Add(const char* message)
	{
		SYSTEMTIME now;
		GetLocalTime(&now);
		char timestamp[16];
		snprintf(timestamp, sizeof(timestamp), "%02d:%02d:%02d", now.wHour, now.wMinute, now.wSecond);

		// Visual Studioの「出力」ウィンドウ
		OutputDebugStringW(detail::Utf8ToWide(std::string(message) + "\n").c_str());

		// メモリ上のバッファ（将来、別スレッドから書かれても安全なようにロックする）
		auto& storage = detail::GetStorage();
		std::lock_guard<std::mutex> lock(storage.mutex);
		storage.entries.push_back({ timestamp, message });
		if (storage.entries.size() > MaxEntries)
		{
			storage.entries.pop_front();
		}
		++storage.version;
	}

	inline uint64_t Version()
	{
		auto& storage = detail::GetStorage();
		std::lock_guard<std::mutex> lock(storage.mutex);
		return storage.version;
	}

	inline std::vector<Entry> Snapshot()
	{
		auto& storage = detail::GetStorage();
		std::lock_guard<std::mutex> lock(storage.mutex);
		return std::vector<Entry>(storage.entries.begin(), storage.entries.end());
	}
}

// ログを出す。printfと同じ書式が使える
// 例：Log("プレハブを読み込めません: %s", path.c_str());
inline void Log(const char* format, ...)
{
	char message[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	LogBuffer::Add(message);
}

// ログに出すパスを短くする：「Data/」から
inline std::string LogPath(const std::string& path)
{
	const std::filesystem::path p(path); // std::string はシステムの文字コード（Shift-JIS）として読まれる

	std::filesystem::path shortPath;
	bool found = false;
	for (const auto& part : p)
	{
		if (!found && part == "Data") found = true;
		if (found) shortPath /= part;
	}
	if (!found) shortPath = p.filename();

	const auto utf8 = shortPath.generic_u8string(); // 区切りは「/」、文字は UTF-8
	return std::string(utf8.begin(), utf8.end());
}