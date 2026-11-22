#pragma once
#include <string>

// このエンティティがどのプレハブから生成されたかを記録する
struct PrefabInstanceComponent
{
	std::string prefabPath;
};