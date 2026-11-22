#pragma once
#include <string>
#include <Windows.h>

// Windowsのファイルダイアログ
namespace FileDialog
{
	bool SaveScene(HWND owner, std::string& path);

	bool OpenScene(HWND owner, std::string & path);
}