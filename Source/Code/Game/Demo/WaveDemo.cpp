#include "WaveDemo.h"

void WaveDemo::Initialize(
	ID3D11Device* device,
	entt::registry& registry,
	int gridSize,
	float spacingValue)
{
	renderer = ShapeRenderer(device);
	spacing = spacingValue;

	for (int x = 0; x < gridSize; x++)
	{
		for (int z = 0; z < gridSize; z++)
		{
			entt::entity e = registry.create();

			Tag tag;
			tag.baseX = (x - gridSize * 0.5f) * spacing;
			tag.baseZ = (z - gridSize * 0.5f) * spacing;
			tag.phase = (tag.baseX + tag.baseZ) * 0.3f;

			registry.emplace<Tag>(e, tag);
		}
	}
}

void WaveDemo::Render(
	entt::registry& registry,
	ID3D11DeviceContext* dc,
	const DirectX::XMFLOAT4X4& view, 
	const DirectX::XMFLOAT4X4& projection, 
	const DirectX::XMFLOAT3& lightDirection)
{
	auto view_ = registry.view<Tag>();
	for (auto entity : view_)
	{
		auto& tag = view_.get<Tag>(entity);

		float y = sinf(time * 2.0f + tag.phase) * 0.5f;

		DirectX::XMMATRIX m = DirectX::XMMatrixTranslation(tag.baseX, y, tag.baseZ);
		DirectX::XMFLOAT4X4 transform;
		DirectX::XMStoreFloat4x4(&transform, m);

		DirectX::XMFLOAT3 size = { spacing, spacing, spacing };

		float seed = sinf(tag.baseX * 12.9898f + tag.baseZ * 78.233f) * 43758.5453f;
		float variation = seed - floorf(seed);

		DirectX::XMFLOAT4 color = {
			0.75f + variation * 0.12f,
			0.88f - variation * 0.06f,
			1.0f - variation * 0.03f,
			1.0f
		};

		renderer.DrawBox(transform, size, color);
	}

	renderer.Render(dc, view, projection, lightDirection, false);
}