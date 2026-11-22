#include "FileDialog.h"
#include <commdlg.h>
#include <filesystem>

#pragma comment(lib, "comdlg32.lib")

namespace
{
	bool EndsWith(const std::string& text, const std::string& suffix)
	{
		return text.size() >= suffix.size()
			&& text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
	}

	// 拡張子を「.scene.json」に揃える
	std::string EnsureSceneExtension(std::string path)
	{
		const std::string sceneExtension = ".scene.json";
		if (EndsWith(path, sceneExtension)) return path;
		if (EndsWith(path, ".json")) path.erase(path.size() - 5);
		return path + sceneExtension;
	}
}

namespace FileDialog
{
	bool SaveScene(HWND owner, std::string& path)
	{
		// 保存先の初期フォルダ（無ければ作る）。ダイアログには絶対パスで渡す
		std::error_code ec;
		std::filesystem::create_directories("Data/Scene", ec);
		std::string initialDir = std::filesystem::absolute("Data/Scene").string();

		char fileName[MAX_PATH] = {};
		strncpy_s(fileName, path.c_str(), _TRUNCATE);

		OPENFILENAMEA ofn = {};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = owner;
		ofn.lpstrFilter = "Scene (*.scene.json)\0*.scene.json\0";
		ofn.lpstrFile = fileName;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrInitialDir = initialDir.c_str();
		ofn.lpstrTitle = "Save Scene";
		// OFN_NOCHANGEDIR：ダイアログが、カレントディレクトリを変えないようにする（相対パスを使っているため）
		ofn.Flags = OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;

		if (!GetSaveFileNameA(&ofn)) return false;

		std::string selected = EnsureSceneExtension(fileName);

		// 拡張子を補った後のファイルが既にあれば、上書きの確認を出す
		// （ダイアログ標準の確認は、補う前の名前で判定されるため、使わない）
		if (std::filesystem::exists(selected))
		{
			std::string message = "'" + std::filesystem::path(selected).filename().string()
				+ "' already exists.\nDo you want to replace it?";
			if (MessageBoxA(owner, message.c_str(), "Save Scene", MB_YESNO | MB_ICONWARNING) != IDYES)
			{
				return false;
			}
		}

		path = selected;
		return true;
	}
	bool OpenScene(HWND owner, std::string& path)
	{
		std::error_code ec;
		std::filesystem::create_directories("Data/Scene", ec);
		std::string initialDir = std::filesystem::absolute("Data/Scene").string();

		char fileName[MAX_PATH] = {};

		OPENFILENAMEA ofn = {};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = owner;
		ofn.lpstrFilter = "Scene (*.scene.json)\0*.scene.json\0";
		ofn.lpstrFile = fileName;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrInitialDir = initialDir.c_str();
		ofn.lpstrTitle = "Open Scene";
		ofn.Flags = OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		if (!GetOpenFileNameA(&ofn)) return false;

		path = fileName;
		return true;
	}
}