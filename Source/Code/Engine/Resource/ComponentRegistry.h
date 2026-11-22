#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include "entt.hpp"

struct ComponentInfo
{
    std::string typeName;    // 識別子(コード上の型名)。保存のキーにも使う
    std::string displayName; // エディタ上の表示名
    std::function<void(entt::registry&, entt::entity)> add;
    std::function<void(entt::registry&, entt::entity)> remove;
    std::function<bool(entt::registry&, entt::entity)> has;
    // エンティティが持つコンポーネントの実態をmeta_anyとして取得する
    std::function<entt::meta_any(entt::registry&, entt::entity)> getMeta;
};

class ComponentRegistry
{
public:
    static void Register(ComponentInfo info);
    static const std::vector<std::string>& GetTypeNames();
    static const ComponentInfo* Get(const std::string& typeName);

    // 型1つ分を、標準の付け外し処理で登録する
    template<typename T>
    static void RegisterType(const std::string& typeName, const std::string& displayName)
    {
        Register({
            typeName,
            displayName,
            [](entt::registry& r, entt::entity e) { if (!r.any_of<T>(e)) r.emplace<T>(e); },
            [](entt::registry& r, entt::entity e) { r.remove<T>(e); },
            [](entt::registry& r, entt::entity e) { return r.any_of<T>(e); },
            [](entt::registry& r, entt::entity e) -> entt::meta_any
            {
                auto* c = r.try_get<T>(e);
                if (!c) return entt::meta_any{};
                return entt::forward_as_meta(*c);
            }
            });
    }

private:
    static std::unordered_map<std::string, ComponentInfo> registry;
    static std::vector<std::string> typeNames;
};

// 型名を、コード上の名前から自動で付けて登録する
#define REGISTER_COMPONENT_TYPE(Type, DisplayName) \
    ComponentRegistry::RegisterType<Type>(#Type, DisplayName)