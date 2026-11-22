#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "entt.hpp"
#include "StudioContext.h"

class IStudioAttachment
{
public:
	virtual ~IStudioAttachment() = default;

    // 更新
    virtual void Update(float elapsedTime, StudioContext& context) {}

    // 3D描画
    virtual void OnRender(
        ID3D11DeviceContext* dc,
        const DirectX::XMFLOAT4X4& view,
        const DirectX::XMFLOAT4X4& projection,
        entt::registry& registry,
        StudioContext& context) 
    {}

    // ImGui描画
    virtual void DrawGUI(
        entt::registry& registry,
        StudioContext& context)
    {}
};