#pragma once
#include <memory>
#include "Engine/Resource/Model.h"

// 静的メッシュ用コンポーネント
struct StaticMeshComponent
{
    // モデルデータ
    std::shared_ptr<Model> model; 
};