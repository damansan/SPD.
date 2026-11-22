#pragma once
#include <d3d11.h>
#include "Engine/Rendering/SkyBox.h"
#include "Engine/Resource/ResourceManager.h"

struct StudioSharedContext
{
    SkyBox          skyBox;
    ResourceManager resourceManager;

    // ポストエフェクトの設定
    const PostProcessSettings* postProcessSettings = nullptr;

    void Initialize(ID3D11Device* device)
    {
        skyBox.Initialize(
            device, 		
            //"Data/SkyBox/ToolSky"
            "Data/SkyBox/DynamicSky"
        );
    }
};