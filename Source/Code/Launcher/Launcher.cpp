#include <windows.h>
#include <commctrl.h>
#include <string>

// タスクダイアログ(ボタン付きの選択画面)を使うための設定
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace
{
	// 選択画面のボタンのID
	enum
	{
		kButtonEditor = 100,
		kButtonGame = 101,
	};

	// 起動するアプリを選択画面で選ぶ
	int ShowSelectDialog()
	{
		TASKDIALOG_BUTTON buttons[] =
		{
			{ kButtonEditor, L"Editor\nエディタ付きで起動する" },
			{ kButtonGame,   L"Game\nゲーム単体で起動する" },
		};

		TASKDIALOGCONFIG config = {};
		config.cbSize = sizeof(config);
		config.dwFlags = TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION;
		config.pszWindowTitle = L"Launcher";
		config.pszMainInstruction = L"起動するアプリを選んでください";
		config.pButtons = buttons;
		config.cButtons = ARRAYSIZE(buttons);
		config.nDefaultButton = kButtonEditor;

		int selected = 0;
		if (FAILED(TaskDialogIndirect(&config, &selected, nullptr, nullptr))) return 0;

		return (selected == kButtonEditor || selected == kButtonGame) ? selected : 0;
	}

	// ランチャーと同じフォルダにあるファイルのパスを作る
	std::wstring GetSiblingPath(const wchar_t* fileName)
	{
		wchar_t path[MAX_PATH] = {};
		GetModuleFileNameW(nullptr, path, MAX_PATH);

		std::wstring dir(path);
		dir = dir.substr(0, dir.find_last_of(L"\\/") + 1);
		return dir + fileName;
	}

	// 別プロセスとして起動する
	bool Launch(const std::wstring& exePath)
	{
		STARTUPINFOW si = {};
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi = {};

		std::wstring commandLine = L"\"" + exePath + L"\"";
		if (!CreateProcessW(exePath.c_str(), commandLine.data(),
			nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
		{
			return false;
		}

		// 起動したプロセスの終了は待たないので、ハンドルはすぐ閉じる
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		return true;
	}
}

int WINAPI wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
	int selected = ShowSelectDialog();
	if (selected == 0) return 0;

	const wchar_t* exeName = (selected == kButtonEditor) ? L"EditorApp.exe" : L"GameApp.exe";
	std::wstring exePath = GetSiblingPath(exeName);

	if (!Launch(exePath))
	{
		std::wstring message = L"起動できませんでした:\n" + exePath;
		MessageBoxW(nullptr, message.c_str(), L"3dgp", MB_OK | MB_ICONERROR);
		return 1;
	}
	return 0;
}