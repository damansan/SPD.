#include "ComponentRegistry.h"

std::unordered_map<std::string, ComponentInfo> ComponentRegistry::registry;
std::vector<std::string> ComponentRegistry::typeNames;

void ComponentRegistry::Register(ComponentInfo info)
{
    if (registry.find(info.typeName) != registry.end()) return;
    typeNames.push_back(info.typeName);
    registry[info.typeName] = std::move(info);
}

const std::vector<std::string>& ComponentRegistry::GetTypeNames()
{
    return typeNames;
}

const ComponentInfo* ComponentRegistry::Get(const std::string& typeName)
{
    auto it = registry.find(typeName);
    if (it == registry.end()) return nullptr;
    return &it->second;
}