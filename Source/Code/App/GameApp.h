#pragma once
#include <string>
#include "Engine/Core/Application.h"

// ゲーム単体のアプリケーション
class GameApp : public Application
{
public:
	using Application::Application;

protected:
	void OnInitialize() override;
	void OnPreUpdate(float elapsedTime) override;
	void OnPostUpdate(float elapsedTime) override;
	void OnPresent() override;

private:
	// プロジェクト設定から、最初に読み込むシーンのパスを取得する
	static std::string LoadStartScenePath();

	// 最初のシーンファイル
	std::string startScenePath;
	bool startSceneLoaded = false;
};