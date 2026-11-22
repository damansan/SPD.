#include "ComponentInspectorRegistry.h"

std::unordered_map<std::string, std::function<void(entt::registry&, entt::entity)>> ComponentInspectorRegistry::registry;

void ComponentInspectorRegistry::Register(const std::string& name, std::function<void(entt::registry&, entt::entity)> drawImGui)
{
	registry[name] = std::move(drawImGui);
}

const std::function<void(entt::registry&, entt::entity)>* ComponentInspectorRegistry::Get(const std::string& name)
{
	auto it = registry.find(name);
	if (it == registry.end()) return nullptr;
	return &it->second;
}