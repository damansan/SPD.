#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include "entt.hpp"

class ComponentInspectorRegistry
{
public:
	static void Register(const std::string& typeName, std::function<void(entt::registry&, entt::entity)> drawImGui);
	static const std::function<void(entt::registry&, entt::entity)>* Get(const std::string& typeName);

private:
	static std::unordered_map<std::string, std::function<void(entt::registry&, entt::entity)>> registry;
};

//// 型名を、コード上の名前から自動で付けて独自描画を登録する
//#define REGISTER_INSPECTOR(Type, DrawFunc) \
//    ComponentInspectorRegistry::Register(#Type, DrawFunc)

// 描画関数(ラムダ)の中に { } で囲まれたカンマがあってもよいように、可変長引数で受け取る
#define REGISTER_INSPECTOR(Type, ...) \
    ComponentInspectorRegistry::Register(#Type, __VA_ARGS__)