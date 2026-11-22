#pragma once
#include <entt.hpp>
#include <d3d11.h>
#include <DirectXMath.h>
#include "Engine/Rendering/ShapeRenderer.h"

class WaveDemo
{
public:
	void Initialize(
		ID3D11Device* device,
		entt::registry& registry,
		int gridSize,
		float spacing);

	void Update(float elapsedTime) { time += elapsedTime; }

	void Render(
		entt::registry& registry,
		ID3D11DeviceContext* dc,
		const DirectX::XMFLOAT4X4& view,
		const DirectX::XMFLOAT4X4& projection,
		const DirectX::XMFLOAT3& lightDirection);

private:
	struct Tag
	{
		float baseX = 0.0f;
		float baseZ = 0.0f;
		float phase = 0.0f;
	};

	ShapeRenderer renderer;
	float time = 0.0f;
	float spacing = 1.0f;
};