#pragma once
#include <cstdint>
#include "Engine/ECS/Reflect.h"

// 使用するGBufferシェーダーの種類
enum class ShadingModel : uint8_t
{
    Default = 0,
    Basic,
    Grid,
};

REFLECT_ENUM(ShadingModel)

    REFLECT_ENUM_VALUE(Default, ShadingModel::Default, "Default (PBR)")
    REFLECT_ENUM_VALUE(Basic, ShadingModel::Basic, "Basic")
    REFLECT_ENUM_VALUE(Grid, ShadingModel::Grid, "Grid ")

REFLECT_ENUM_END()

struct MaterialComponent
{
    REFLECT_BEGIN(MaterialComponent, "Material")
    
        REFLECT_FIELD(ShadingModel, shadingModel, (ShadingModel::Default), ("Shader"))
    
    REFLECT_END()

    // シェーダー名に変換するヘルパー
    const char* GetShaderName() const
    {
        switch (shadingModel)
        {
        case ShadingModel::Basic:   return "gbuffer_basic";
        case ShadingModel::Grid:    return "gbuffer_grid";
        default:                    return "gbuffer_default";
        }
    }
};