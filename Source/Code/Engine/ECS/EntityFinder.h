#pragma once
#include "entt.hpp"
#include "Engine/ECS/Components/CategoryComponent.h"

#include <vector>

// 指定した属性を持つ最初のEntityを列挙
inline entt::entity FindFirstByCategory(entt::registry& registry, EntityCategory category)
{
	auto view = registry.view<CategoryComponent>();
	for (auto entity : view)
	{
		if (view.get<CategoryComponent>(entity).category == category)
		{
			return entity;
		}
	}
	return entt::null;
}

// 指定した属性を持つ全部のEntityを列挙
inline std::vector<entt::entity> FindAllByCategory(entt::registry& registry, EntityCategory category)
{
	std::vector<entt::entity> result;

	auto view = registry.view<CategoryComponent>();
	for (auto entity : view)
	{
		if (view.get<CategoryComponent>(entity).category == category)
		{
			result.push_back(entity);
		}
	}
	return result;
}